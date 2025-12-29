// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/tools/elf2efi.c
 *	Copyright (c) 2025 Yao Zi <me@ziyao.cc>
 *
 *	Convert an ELF file that contains *_RELATIVE relocations only to a PE32+
 *	file with subsystem = IMAGE_SUBSYSTEM_EFI_APPLICATION.
 *
 *	Note this program assumes the host is little-endian.
 */

/*
 * 1. Iterate over the program headers of the ELF file
 *
 * 2. Check the .dynamic segment (PT_DYNAMIC). Iterate over DT_RELA* and DT_REL*
 *    to find out relocations that must be applied. Bail out when encountering
 *    any relocations other than *_RELATIVE, which are probably couldn't be
 *    represented through base relocation table.
 *
 * 3. Determine the memory layout. Non-PT_LOAD segments could be ignored.
 *
 * 3. Apply all relocations for the data.
 *
 * 4. Craft a .reloc PE base relocation section from the ELF relocations.
 *
 * 5. Write PE32+ headers, section headers, and data with relocations applied.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <unistd.h>

#pragma pack(push, 1)

typedef struct {
	uint8_t magic[2];
	uint8_t pad[58];
	uint32_t pe_offset;
} PE_Dos_Header;

typedef struct {
	uint8_t signature[4];
	uint16_t machine;
	uint16_t numberOfSections;
	uint32_t timedateStamp;
	uint32_t pointerToSymbolTable;
	uint32_t numberOfSymbols;
	uint16_t sizeOfOptionalHeader;
	uint16_t characteristics;
} PE_Header;

#define PE_IMAGE_MACHINE_AMD64		0x8664

#define PE_IMAGE_EXECUTABLE_IMAGE	0x0002
#define PE_IMAGE_LINE_NUMS_STRIPPED	0x0004
#define PE_IMAGE_LOCAL_SYMS_STRIPPED	0x0008
#define PE_IMAGE_LARGE_ADDRESS_AWARE	0x0020
#define PE_IMAGE_FILE_DEBUG_STRIPPED	0x0200

typedef struct {
	uint16_t magic;
	uint8_t majorLinkerVersion;
	uint8_t minorLinkerVersion;
	uint32_t sizeOfCode;
	uint32_t sizeOfInitializedData;
	uint32_t sizeOfUninitializedData;
	uint32_t addressOfEntryPoint;
	uint32_t baseOfCode;
} PE32Plus_Optional_Header;

typedef struct {
	uint64_t imageBase;
	uint32_t sectionAlignment;
	uint32_t fileAlignment;
	uint16_t majorOSVersion;
	uint16_t minorOSVersion;
	uint16_t majorImageVersion;
	uint16_t minorImageVersion;
	uint16_t majorSubsysVersion;
	uint16_t minorSubsysVersion;
	uint32_t win32VersionValue;
	uint32_t sizeOfImage;
	uint32_t sizeOfHeaders;
	uint32_t checkSum;
	uint16_t subsystem;
	uint16_t dllCharacteristics;
	uint64_t sizeOfStackReserve;
	uint64_t sizeOfStackCommit;
	uint64_t sizeOfHeapReserve;
	uint64_t sizeOfHeapCommit;
	uint32_t loaderFlags;
	uint32_t numberOfRvaAndSizes;
} PE32Plus_Optional_Windows_Header;

#define IMAGE_SUBSYSTEM_EFI_APPLICATION		10

#define IMAGE_DLLCHAR_HIGH_ENTROPY_VA		0x0020
#define IMAGE_DLLCHAR_DYNAMIC_BASE		0x0040
#define IMAGE_DLLCHAR_NX_COMPAT			0x0100

typedef struct {
	uint32_t rva;
	uint32_t size;
} PE32_Data_Directory;

typedef PE32_Data_Directory PE32Plus_Data_Directory;

typedef struct {
	PE32_Data_Directory export;
	PE32_Data_Directory import;
	PE32_Data_Directory resource;
	PE32_Data_Directory exception;
	PE32_Data_Directory certificate;
	PE32_Data_Directory baseRelocation;
	PE32_Data_Directory debug;
	PE32_Data_Directory architecture;
	PE32_Data_Directory globalPointer;
	PE32_Data_Directory tls;
	PE32_Data_Directory loadConfig;
	PE32_Data_Directory boundImport;
	PE32_Data_Directory importAddress;
	PE32_Data_Directory delayImport;
	PE32_Data_Directory clrRuntime;
	PE32_Data_Directory reserved;
} PE32_Optional_Data_Directory_Header;

typedef PE32_Optional_Data_Directory_Header \
	PE32Plus_Optional_Data_Directory_Header;

typedef struct {
	char		name[8];
	uint32_t	virtualSize;
	uint32_t	virtualAddress;
	uint32_t	sizeOfRawData;
	uint32_t	pointerToRawData;
	uint32_t	pointerToRelocations;
	uint32_t	pointerToLineNumbers;
	uint16_t	numberOfRelocations;
	uint16_t	numberOfLineNumbers;
	uint32_t	characteristics;
} PE32_Section_Header;

#define PE32_SCN_CNT_CODE			0x00000020
#define PE32_SCN_CNT_INITIALIZED_DATA		0x00000040
#define PE32_SCN_CNT_UNINITIALIZED_DATA		0x00000080
#define PE32_SCN_MEM_DISCARDABLE		0x02000000
#define PE32_SCN_MEM_EXECUTE			0x20000000
#define PE32_SCN_MEM_READ			0x40000000
#define PE32_SCN_MEM_WRITE			0x80000000

typedef PE32_Section_Header PE32Plus_Section_Header;

typedef struct {
	uint32_t rva;
	uint32_t size;
} PE32_Base_Reloc_Block;

typedef uint16_t PE32_Base_Reloc_Entry;

#define PE32_BASE_RELOC_RAW(type, offset)	((type) << 12 | (offset))

#define PE32_IMAGE_REL_BASED_DIR64		10

#pragma pack(pop)

#define PE32Plus_Image_Header_Size(nsections) (\
	sizeof(PE_Dos_Header) + sizeof(PE_Header) +		\
	sizeof(PE32Plus_Optional_Header) +			\
	sizeof(PE32Plus_Optional_Windows_Header) +		\
	sizeof(PE32_Optional_Data_Directory_Header) +		\
	sizeof(PE32_Section_Header) * (nsections))

#define ALIGN_DOWN(x, align)	((x) / (align) * (align))
#define ALIGN(x, align)		(ALIGN_DOWN(x + align - 1, align))

#define do_if(cond, stmt) do { \
	if (cond) {							\
		stmt;							\
	}								\
} while (0)

#define assert_msg(cond, ...) \
	do_if(!(cond), {						\
		fprintf(stderr, __VA_ARGS__);				\
		exit(1);						\
	})

static int gVerbosity;

static void
verbose(const char *fmt, ...)
{
	if (!gVerbosity)
		return;

	va_list va;
	va_start(va, fmt);

	vfprintf(stderr, fmt, va);

	va_end(va);
}

static void
debug(const char *fmt, ...)
{
	if (gVerbosity < 2)
		return;

	va_list va;
	va_start(va, fmt);

	vfprintf(stderr, fmt, va);

	va_end(va);
}

static void *
malloc_checked(size_t size)
{
	void *p = malloc(size);

	if (!p) {
		fputs("failed to allocate memory\n", stderr);
		exit(1);
	}

	return p;
}

static void *
realloc_checked(void *p, size_t size)
{
	void *n = realloc(p, size);

	if (!n) {
		fputs("failed to allocate memory\n", stderr);
		exit(1);
	}

	return n;
}

static char *
strdup_checked(const char *s)
{
	char *p = strdup(s);

	if (!p) {
		fputs("failed to allocate memory\n", stderr);
		exit(1);
	}

	return p;
}

typedef struct {
	char *name;
	uint8_t *rawData;
	size_t size;
} Sized_Data;

static Sized_Data
load_binary_file(const char *path)
{
	FILE *fp = fopen(path, "rb");
	assert_msg(fp, "failed to open file \"%s\"\n", path);

	int ret = fseek(fp, 0, SEEK_END);
	assert_msg(!ret, "failed to seek on file \"%s\"\n", path);

	long int size = ftell(fp);
	assert_msg(size > 0, "failed to get size of file \"%s\"\n", path);

	rewind(fp);

	void *data = malloc_checked(size);
	ret = fread(data, size, 1, fp);
	assert_msg(ret == 1, "failed to read from file \"%s\"\n", path);

	fclose(fp);

	Sized_Data file = {
		.name		= strdup_checked(path),
		.rawData	= data,
		.size		= size,
	};

	return file;
}

static void *
data_at_offset(Sized_Data *data, const char *purpose, size_t off, size_t size)
{
	assert_msg(off + size < data->size,
		   "Attempt to index \"%s\" at %lu with size %lu, "
		   "but data is truncated to %lu\n",
		   data->name, (unsigned long)off, (unsigned long)size,
		   data->size);

	return data->rawData + off;
}

static void
free_sized_data(Sized_Data *data)
{
	free(data->name);
	free(data->rawData);

	data->name	= NULL;
	data->rawData	= NULL;
}

static void
myasprintf(char **strp, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	char buf;
	int size = vsnprintf(&buf, 1, fmt, va);
	va_end(va);

	*strp = malloc_checked(size + 1);

	va_start(va, fmt);
	vsnprintf(*strp, size + 1, fmt, va);
	va_end(va);
}

/* Segment (ELF semantics) */
#define SEGMENT_FLAGS_READ	0x1
#define SEGMENT_FLAGS_WRITE	0x2
#define SEGMENT_FLAGS_EXEC	0x4
typedef struct {
	uint64_t vaddr;
	size_t memsize;

	uint32_t alignment;
	uint32_t flags;

	size_t filesize;
	Sized_Data data;
} Segment;

static Segment
load_segment(Sized_Data *elf, Elf64_Phdr *phdr)
{
	uint32_t flags = phdr->p_flags;

	assert_msg(phdr->p_memsz >= phdr->p_filesz,
		   "invalid in-memory size %lu for segment at offset 0x%lx, "
		   "which is smaller than in-file size %lu\n",
		   (unsigned long)(phdr->p_memsz),
		   (unsigned long)(phdr->p_offset),
		   (unsigned long)(phdr->p_filesz));

	debug("loading segment at offset 0x%lx, size %lu, vaddr = 0x%lx\n",
	      (unsigned long)(phdr->p_offset),
	      (unsigned long)(phdr->p_filesz),
	      (unsigned long)(phdr->p_vaddr));

	Segment segment = {
		.vaddr		= phdr->p_vaddr,
		.memsize	= phdr->p_memsz,

		.alignment	= phdr->p_align,
		.flags		= ((flags & PF_R) ? SEGMENT_FLAGS_READ : 0)  |
				  ((flags & PF_W) ? SEGMENT_FLAGS_WRITE : 0) |
				  ((flags & PF_X) ? SEGMENT_FLAGS_EXEC : 0),

		.filesize	= phdr->p_filesz,
	};

	void *fileData = data_at_offset(elf, "segment data",
					phdr->p_offset, phdr->p_filesz);

	/*
	 * Make a copy of the original data to match in-memory size instead of
	 * in-file size. We may want to perform relocations against the
	 * uninitialized (larger than in-file size) later.
	 */
	uint8_t *dataCopy = malloc_checked(phdr->p_memsz);
	memcpy(dataCopy, fileData, phdr->p_filesz);
	memset(dataCopy + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_memsz);

	myasprintf(&segment.data.name, "segment at offset 0x%lx",
		   phdr->p_offset);
	segment.data.rawData = dataCopy;
	segment.data.size = phdr->p_memsz;

	return segment;
}

static Segment *
load_segments(Sized_Data *elf, Elf64_Ehdr *ehdr,
	      size_t *segnum, Segment *dynamic)
{
	size_t phdrSize = ehdr->e_phnum * sizeof(Elf64_Phdr);
	Elf64_Phdr *phdrs = data_at_offset(elf, "PHDR", ehdr->e_phoff,
					   phdrSize);

	*segnum = 0;
	Segment *segments = malloc_checked(sizeof(Segment) * ehdr->e_phnum);
	bool foundDynamic = false;

	for (unsigned int i = 0; i < ehdr->e_phnum; i++) {
		Elf64_Phdr *phdr = phdrs + i;

		switch (phdr->p_type) {
		case PT_DYNAMIC:
			verbose("found PT_DYNAMIC segment at index %u\n", i);
			*dynamic = load_segment(elf, phdr);
			foundDynamic = true;
			continue;
		case PT_LOAD:
			verbose("found PT_LOAD segment at index %u\n", i);
			segments[*segnum] = load_segment(elf, phdr);
			(*segnum)++;
			break;
		default:
			debug("ignoring segment %u, type = 0x%x, "
			      "file offset 0x%lx\n",
			      i, phdr->p_type, phdr->p_vaddr);
			break;
		}
	}

	assert_msg(foundDynamic, "no PT_DYNAMIC segment found\n");

	verbose("%lu segments in total, parsed %u\n",
		ehdr->e_phnum, *segnum + 1);

	return segments;
}

static void
valid_ehdr(Elf64_Ehdr *ehdr)
{
	assert_msg(!memcmp(&ehdr->e_ident, "\x7f" "ELF", 4),
		   "invalid ELF header\n");

	assert_msg(ehdr->e_ident[EI_CLASS] == ELFCLASS64,
		   "Only 64-bit ELF files are supported, "
		   "got 0x%x in EI_CLASS\n",
		   ehdr->e_ident[EI_CLASS]);

	assert_msg(ehdr->e_ident[EI_DATA] == ELFDATA2LSB,
		   "Only little endian ELF files are supported, "
		   "got 0x%x in EI_DATA\n",
		   ehdr->e_ident[EI_DATA]);

	// TODO: Should we ban ET_EXEC?
	assert_msg(ehdr->e_type == ET_DYN || ehdr->e_type == ET_EXEC,
		   "Require executable or shared object files, "
		   "got e_type == %x\n",
		   ehdr->e_type);

	assert_msg(ehdr->e_machine == EM_X86_64,
		   "Unsupporte machine type 0x%x\n",
		   ehdr->e_machine);

	assert_msg(ehdr->e_ehsize == sizeof(Elf64_Ehdr),
		   "Unsupported ELF EHDR size %u\n",
		   ehdr->e_ehsize);

	assert_msg(ehdr->e_phentsize == sizeof(Elf64_Phdr),
		   "Unsupported ELF PHDR size %u\n",
		   ehdr->e_phentsize);
}

static bool
dynamic_lookup_tag(Segment *dynamic, uint64_t tag, uint64_t *value)
{
	for (unsigned int i = 0;
	     i < dynamic->filesize / sizeof(Elf64_Dyn);
	     i++) {
		Elf64_Dyn *dyn = data_at_offset(&dynamic->data, "dynamic tag",
						i, sizeof(Elf64_Dyn));

		if (dyn->d_tag == tag) {
			*value = dyn->d_un.d_val;
			return true;
		}
	}

	return false;
}

typedef struct {
	uint64_t vaddr;
	uint64_t addend;
} Exec_Reloc;

typedef struct {
	uint64_t entryAddr;

	Segment *segments;
	Segment dynamic;
	size_t segnum;

	Exec_Reloc *relocs;
	size_t relocnum;

	uint32_t executableAlignment;
} Executable;

static void *
data_at_vaddr(Executable *exec, const char *purpose, uint64_t vaddr,
	      size_t size, Segment **pSegment)
{
	for (size_t i = 0; i < exec->segnum; i++) {
		Segment *segment = &exec->segments[i];
		if (segment->vaddr <= vaddr &&
		    vaddr <= segment->vaddr + segment->memsize) {
			if (pSegment)
				*pSegment = segment;

			return segment->data.rawData +
			       vaddr - segment->vaddr;
		}
	}

	assert_msg(0, "Attempt to index \"%s\" at virtual address 0x%lx, "
		      "with size %lu, which isn't (fully) covered by any "
		      "segments\n",
		   purpose, (unsigned long)vaddr, (unsigned long)size);

	return NULL;
}

static size_t
load_rela_relocs(Exec_Reloc *output, Elf64_Rela *rela, size_t num)
{
	size_t effectiveNum = 0;

	for (size_t i = 0; i < num; i++) {
		Elf64_Rela *r = &rela[i];

		switch (ELF64_R_TYPE(r->r_info)) {
		case R_X86_64_NONE:
			continue;
		case R_X86_64_RELATIVE:
			break;
		default:
			fprintf(stderr, "Unsupported RELA relocation type %u, "
					"index %lu\n",
				(unsigned int)ELF64_R_TYPE(r->r_info),
				(unsigned long)i);
			exit(1);
		}

		output[i] = (Exec_Reloc) {
			.vaddr	= r->r_offset,
			.addend	= r->r_addend,
		};

		debug("Found relative relocation against 0x%lx, addend 0x%lx\n",
		      output[i].vaddr, output[i].addend);

		effectiveNum++;
	}

	return effectiveNum;
}

static size_t
load_rel_relocs(Executable *exec, Exec_Reloc *output, Elf64_Rel *rel,
		size_t num)
{
	size_t effectiveNum = 0;

	for (size_t i = 0; i < num; i++) {
		Elf64_Rel *r = &rel[i];

		switch (ELF64_R_TYPE(r->r_info)) {
		case R_X86_64_NONE:
			continue;
		case R_X86_64_RELATIVE:
			break;
		default:
			fprintf(stderr, "Unsupported REL relocation type %x, "
					"index %lu\n",
				(unsigned int)ELF64_R_TYPE(r->r_info),
				(unsigned long)i);
			exit(1);
		}

		uint64_t *p_addend = data_at_vaddr(exec, "addend for DT_REL",
						   r->r_offset, 8, NULL);

		output[i] = (Exec_Reloc) {
			.vaddr	= r->r_offset,
			.addend	= *p_addend,
		};

		debug("Found relative relocation against 0x%lx, addend 0x%lx\n",
		      output[i].vaddr, output[i].addend);

		effectiveNum++;
	}

	return effectiveNum;
}

static int reloc_compare(const void *a, const void *b)
{
	const Exec_Reloc *r1 = a, *r2 = b;
	return r1->vaddr - r2->vaddr;
}

static Exec_Reloc *
load_relocs(Executable *exec, Sized_Data *elf, Segment *dynamic,
	    size_t *relocnum)
{
	Exec_Reloc *relocs = NULL;
	uint64_t offset;

	if (dynamic_lookup_tag(dynamic, DT_RELA, &offset)) {
		uint64_t size;
		assert_msg(dynamic_lookup_tag(dynamic, DT_RELASZ, &size),
			   "Missing DT_RELASZ entry paired with DT_RELA\n");
		size_t num = size / sizeof(Elf64_Rela);

		Elf64_Rela *rela = data_at_offset(elf, "RELA reloctions",
						  offset,
						  sizeof(Elf64_Rela) * num);

		relocs = malloc_checked(sizeof(Exec_Reloc) * num);
		*relocnum = load_rela_relocs(relocs, rela, num);

		verbose("RELA relocations are at 0x%lx, %lu entries\n",
			(unsigned long)offset, (unsigned long)num);
	} else {
		verbose("No RELA relocations found\n");
	}

	if (dynamic_lookup_tag(dynamic, DT_REL, &offset)) {
		uint64_t size;
		assert_msg(dynamic_lookup_tag(dynamic, DT_RELSZ, &size),
			   "Missing DT_RELSZ entry paired with DT_REL\n");
		size_t num = size / sizeof(Elf64_Rel);

		Elf64_Rel *rel = data_at_offset(elf, "REL reloctions",
						offset,
						sizeof(Elf64_Rel) * num);

		relocs = realloc_checked(relocs,
					 sizeof(*relocs) * (num + *relocnum));
		*relocnum += load_rel_relocs(exec, relocs + *relocnum,
					     rel, num);

		verbose("REL relocations are at 0x%lx, %lu entries\n",
			(unsigned long)offset, (unsigned long)num);
	} else {
		verbose("No REL relocations found\n");
	}

	qsort(relocs, *relocnum, sizeof(Exec_Reloc), reloc_compare);

	return relocs;
}


static int segment_compare(const void *a, const void *b)
{
	const Segment *s1 = a, *s2 = b;
	return s1->vaddr - s2->vaddr;
}

static uint32_t gcd(uint32_t a, uint32_t b)
{
	return b == 0 ? a : gcd(b, a % b);
}

static uint32_t
exec_address_alignment(Executable *exec)
{
	uint32_t alignGCD = 0;

	for (size_t i = 0; i < exec->segnum; i++)
		alignGCD = gcd(exec->segments[i].alignment, alignGCD);

	uint32_t alignment = alignGCD;
	for (size_t i = 0; i < exec->segnum; i++)
		alignment *= exec->segments[i].alignment / alignGCD;

	return alignment;
}

static Executable
parse_elf(Sized_Data *elf)
{
	Executable exec;
	Elf64_Ehdr *ehdr = data_at_offset(elf, "EHDR", 0, sizeof(Elf64_Ehdr));

	valid_ehdr(ehdr);

	verbose("ELF entry point 0x%lx\n", ehdr->e_entry);
	exec.entryAddr = ehdr->e_entry;

	exec.segments = load_segments(elf, ehdr, &exec.segnum, &exec.dynamic);
	qsort(exec.segments, exec.segnum, sizeof(Segment), segment_compare);

	exec.relocs = load_relocs(&exec, elf, &exec.dynamic, &exec.relocnum);
	verbose("%lu necessary relocations to process\n",
		(unsigned long)exec.relocnum);

	exec.executableAlignment = exec_address_alignment(&exec);

	return exec;
}

static void
relocate_exec(Executable *exec, uint64_t baseAddress)
{
	/*
	 * Advance the addresses provided by ELF only if the space before the
	 * lowest segment couldn't contain the PE32+ header.
	 */
	size_t headerSize = PE32Plus_Image_Header_Size(exec->segnum + 1);
	if (exec->segments[0].vaddr <= headerSize) {
		uint32_t offset = ALIGN(headerSize, exec->executableAlignment);

		verbose("Address of the first section is too low, "
			"adding 0x%x offset\n", offset);

		baseAddress += offset;
	}

	exec->entryAddr += baseAddress;
	for (size_t i = 0; i < exec->segnum; i++) {
		Segment *segment = &exec->segments[i];

		segment->vaddr += baseAddress;

		/* Align down to segment alignment, and expand section data */
		uint64_t alignedAddr = ALIGN_DOWN(segment->vaddr,
						  segment->alignment);
		uint32_t offset = segment->vaddr - alignedAddr;

		uint8_t *newData = malloc_checked(segment->memsize + offset);
		memset(newData, 0, offset);
		memcpy(newData + offset, segment->data.rawData,
		       segment->memsize);

		segment->vaddr		= alignedAddr;
		segment->filesize	+= offset;
		segment->memsize	+= offset;

		free(segment->data.rawData);
		segment->data.rawData	= newData;
		segment->data.size	+= offset;

		debug("segment %lu relocated to 0x%lx\n",
		      (unsigned long)i, segment->vaddr);
	}

	for (size_t i = 0; i < exec->relocnum; i++) {
		exec->relocs[i].vaddr	+= baseAddress;
		exec->relocs[i].addend	+= baseAddress;
	}
}

static void apply_relocs(Executable *exec)
{
	for (size_t i = 0; i < exec->relocnum; i++) {
		Exec_Reloc *reloc = &exec->relocs[i];
		Segment *segment;
		uint64_t *p = data_at_vaddr(exec, "relocation target",
					    reloc->vaddr, 8, &segment);
		*p = reloc->addend;

		debug("Virtual address 0x%lx relocated to value 0x%lx\n",
		      (unsigned long)reloc->vaddr, (unsigned long)*p);

		uint64_t segmentOffset = reloc->vaddr - segment->vaddr;
		if (segmentOffset >= segment->filesize) {
			segment->filesize = segmentOffset + 8;

			debug("Relocation against uninitialized memory area "
			      "extends in-file size of section with virtual "
			      "address 0x%lx to %lu\n",
			      (unsigned long)(segment->vaddr),
			      (unsigned long)(segment->filesize));
		}
	}
}

typedef struct {
	uint8_t *data;
	size_t allocatedSize;
	size_t effectiveSize;
} Dynamic_Buffer;

#define EMPTY_DYNAMIC_BUFFER		{ .data = NULL }

static void
buffer_append(Dynamic_Buffer *buffer, void *data, size_t size)
{
	size_t newSize = buffer->effectiveSize + size;

	if (newSize >= buffer->allocatedSize) {
		buffer->allocatedSize += ALIGN(size, 1024);
		buffer->data = realloc_checked(buffer->data,
					       buffer->allocatedSize);
	}

	memcpy(buffer->data + buffer->effectiveSize, data, size);

	buffer->effectiveSize = newSize;
}

static Sized_Data
create_pe_reloc(Executable *exec, uint64_t baseAddress)
{
	Dynamic_Buffer buf = EMPTY_DYNAMIC_BUFFER;
	uint64_t blockOffset = 0, blockVaddr = 0;
	PE32_Base_Reloc_Block *block;
	size_t blockEntries = 0;

	for (size_t i = 0; i < exec->relocnum; i++) {
		Exec_Reloc *reloc = &exec->relocs[i];
		uint64_t lowerBound = ALIGN_DOWN(reloc->vaddr, 4096);

		/* A new block is necessary */
		if (lowerBound > blockVaddr) {
			/* Backfill the total size of entries */
			block = (PE32_Base_Reloc_Block *)
				(buf.data + blockOffset);

			if (block)
				block->size = blockEntries *
					      sizeof(PE32_Base_Reloc_Entry) +
					      sizeof(PE32_Base_Reloc_Block);

			blockEntries	= 0;
			blockVaddr	= lowerBound;
			blockOffset	= buf.effectiveSize;

			PE32_Base_Reloc_Block newBlock = {
				.rva	= lowerBound - baseAddress,
				.size	= 0,
			};
			buffer_append(&buf, &newBlock, sizeof(newBlock));

			debug("New relocation block, RVA = 0x%lx\n",
			      (unsigned long)(blockVaddr - baseAddress));
		}

		uint16_t entry = PE32_BASE_RELOC_RAW(PE32_IMAGE_REL_BASED_DIR64,
						     reloc->vaddr - blockVaddr);
		buffer_append(&buf, &entry, sizeof(entry));
		blockEntries++;
	}

	if (blockEntries) {
		block = (PE32_Base_Reloc_Block *)(buf.data + blockOffset);
		block->size = blockEntries * sizeof(PE32_Base_Reloc_Entry) +
			      sizeof(PE32_Base_Reloc_Block);
	}

	return (Sized_Data) {
		.name		= strdup_checked("PE relocation block"),
		.rawData	= buf.data,
		.size		= buf.effectiveSize,
	};
}

static void
write_chunk(FILE *fp, void *data, size_t size)
{
	assert_msg(fwrite(data, size, 1, fp) == 1,
		   "failed to write file\n");
}

static void
write_padding(FILE *fp, size_t size)
{
	uint8_t pad[64] = { 0 };

	while (size) {
		size_t written = size > 64 ? 64 : size;
		write_chunk(fp, pad, written);
		size -= written;
	}
}

typedef struct {
	uint32_t sizeOfCode;
	uint32_t sizeOfInitializedData;
	uint32_t sizeOfUninitializedData;
	uint32_t baseOfCode;
} Exec_Stats;

static Exec_Stats
get_exec_stats(Executable *exec, uint64_t baseAddress)
{
	Exec_Stats stats = { 0 };

	for (size_t i = 0; i < exec->segnum; i++) {
		Segment *segment = &exec->segments[i];

		if (segment->flags & SEGMENT_FLAGS_EXEC) {
			stats.sizeOfCode += segment->memsize;
			if (!stats.baseOfCode) {
				stats.baseOfCode = segment->vaddr - baseAddress;
				verbose("PE baseOfCode set to 0x%lx, segment "
					"index %lu\n",
					(unsigned long)(stats.baseOfCode),
					(unsigned long)i);
			}
		} else {
			stats.sizeOfInitializedData	+= segment->filesize;
			stats.sizeOfUninitializedData	+= segment->memsize -
							   segment->filesize;
		}
	}

	verbose("PE code size = %lu, initialized data size = %lu, "
		"uninitialized data size = %lu\n",
		stats.sizeOfCode, stats.sizeOfInitializedData,
		stats.sizeOfUninitializedData);

	return stats;
}

typedef struct {
	uint32_t offset;
	uint32_t size;
} File_Layout;

static File_Layout *
layout_pe_file(Executable *exec, size_t sizeOfHeaders, Sized_Data *peRelocs,
	       uint32_t fileAlignment)
{
	// Allocate one more for .reloc section
	File_Layout *layouts = malloc_checked(sizeof(File_Layout) *
					      (exec->segnum + 1));
	uint32_t offset = sizeOfHeaders;

	for (size_t i = 0; i < exec->segnum; i++) {
		Segment *segment = &exec->segments[i];
		File_Layout *layout = &layouts[i];

		layout->offset	= offset;
		layout->size	= ALIGN(segment->filesize, fileAlignment);

		offset += layout->size;

		debug("PE section at virtual address 0x%lx locates at offset "
		      "0x%lx in file, size = %lu\n",
		      (unsigned long)(segment->vaddr),
		      (unsigned long)(layout->offset),
		      (unsigned long)(layout->size));
	}

	layouts[exec->segnum].offset	= offset;
	layouts[exec->segnum].size	= ALIGN(peRelocs->size, fileAlignment);

	debug("PE .reloc section locates at offset 0x%lx in file, size = %lu\n",
	      (unsigned long)(layouts[exec->segnum].offset),
	      (unsigned long)(layouts[exec->segnum].size));

	return layouts;
}

static void
write_pe(const char *path, Executable *exec, uint64_t baseAddress,
	 Sized_Data *peRelocs)
{
	FILE *out = fopen(path, "wb");
	assert_msg(out, "failed to open file \"%s\" for writing PE output\n",
		   path);

	Exec_Stats stats = get_exec_stats(exec, baseAddress);

	verbose("PE entry point (RVA) = 0x%lx\n",
		(unsigned long)(exec->entryAddr - baseAddress));

	uint32_t fileAlignment = 512;
	assert_msg(exec->executableAlignment % fileAlignment == 0,
		   "Alignment required by ELF (%u) isn't a multiple of %u\n",
		   (unsigned int)(exec->executableAlignment),
		   (unsigned int)fileAlignment);

	size_t sizeOfHeaders = PE32Plus_Image_Header_Size(exec->segnum + 1);
	sizeOfHeaders = ALIGN(sizeOfHeaders, fileAlignment);

	File_Layout *layouts = layout_pe_file(exec, sizeOfHeaders, peRelocs,
					      fileAlignment);

	File_Layout *lastLayout = &layouts[exec->segnum];

	Segment *lastSegment = &exec->segments[exec->segnum - 1];
	uint64_t relocVaddr = ALIGN(lastSegment->vaddr + lastSegment->memsize,
				    exec->executableAlignment);
	verbose("PE .reloc section is given vaddr 0x%lx\n",
		(unsigned long)relocVaddr);

	size_t sizeOfImage = relocVaddr + peRelocs->size - baseAddress;
	sizeOfImage = ALIGN(sizeOfImage, exec->executableAlignment);

	write_chunk(out, &(PE_Dos_Header) {
			.magic		= { 'M', 'Z' },
			.pe_offset	= sizeof(PE_Dos_Header),
		    }, sizeof(PE_Dos_Header));

	write_chunk(out, &(PE_Header) {
		.signature		= { 'P', 'E', '\0', '\0' },
		.machine		= PE_IMAGE_MACHINE_AMD64,
		// One extra section for .reloc
		.numberOfSections	= exec->segnum + 1,
		.timedateStamp		= 0,
		.pointerToSymbolTable	= 0,
		.numberOfSymbols	= 0,
		.sizeOfOptionalHeader	= sizeof(PE32Plus_Optional_Header) +
					  sizeof(PE32Plus_Optional_Windows_Header) +
					  sizeof(PE32Plus_Optional_Data_Directory_Header),
		.characteristics	= PE_IMAGE_EXECUTABLE_IMAGE	|
					  PE_IMAGE_LINE_NUMS_STRIPPED	|
					  PE_IMAGE_LOCAL_SYMS_STRIPPED	|
					  PE_IMAGE_LARGE_ADDRESS_AWARE	|
					  PE_IMAGE_FILE_DEBUG_STRIPPED,
	}, sizeof(PE_Header));


	write_chunk(out, &(PE32Plus_Optional_Header) {
		// PE32+ optional header
		.magic			= 0x20b,
		.majorLinkerVersion	= 0,
		.minorLinkerVersion	= 0,
		.sizeOfCode		= stats.sizeOfCode,
		.sizeOfInitializedData	= stats.sizeOfInitializedData,
		.sizeOfUninitializedData = stats.sizeOfUninitializedData,
		.addressOfEntryPoint	= exec->entryAddr - baseAddress,
		.baseOfCode		= stats.baseOfCode,
	}, sizeof(PE32Plus_Optional_Header));


	write_chunk(out, &(PE32Plus_Optional_Windows_Header) {
		.imageBase		= baseAddress,
		.sectionAlignment	= exec->executableAlignment,
		.fileAlignment		= fileAlignment,
		.majorOSVersion		= 0,
		.minorOSVersion		= 0,
		.majorImageVersion	= 0,
		.minorImageVersion	= 0,
		.majorSubsysVersion	= 0,
		.minorSubsysVersion	= 0,
		.win32VersionValue	= 0,
		.sizeOfImage		= sizeOfImage,
		.sizeOfHeaders		= sizeOfHeaders,
		.checkSum		= 0,
		.subsystem		= IMAGE_SUBSYSTEM_EFI_APPLICATION,
		.dllCharacteristics	= IMAGE_DLLCHAR_HIGH_ENTROPY_VA	|
					  IMAGE_DLLCHAR_DYNAMIC_BASE	|
					  IMAGE_DLLCHAR_NX_COMPAT,
		.sizeOfStackReserve	= 128 * 1024,
		.sizeOfStackCommit	= 16 * 1024,
		.sizeOfHeapReserve	= 1 * 1024 * 1024,
		.sizeOfHeapCommit	= 128 * 1024,
		.loaderFlags		= 0,
		.numberOfRvaAndSizes	=
			sizeof(PE32Plus_Optional_Data_Directory_Header) /
			sizeof(PE32Plus_Data_Directory),
	}, sizeof(PE32Plus_Optional_Windows_Header));

	write_chunk(out, &(PE32Plus_Optional_Data_Directory_Header) {
		.baseRelocation	= {
			.rva	= relocVaddr - baseAddress,
			.size	= peRelocs->size,
		},
	}, sizeof(PE32_Optional_Data_Directory_Header));

	for (size_t i = 0; i < exec->segnum; i++) {
		Segment *segment = &exec->segments[i];

		char segname[9];
		snprintf(segname, sizeof(segname), ".seg%u", (unsigned int)i);

		PE32Plus_Section_Header sheader = {
			.virtualSize		= segment->memsize,
			.virtualAddress		= segment->vaddr - baseAddress,
			.sizeOfRawData		= ALIGN(segment->filesize,
							fileAlignment),
			.pointerToRawData	= segment->filesize ?
							layouts[i].offset : 0,
			.pointerToRelocations	= 0,
			.pointerToLineNumbers	= 0,
			.numberOfRelocations	= 0,
			.numberOfLineNumbers	= 0,
			.characteristics	=
				segment->flags & SEGMENT_FLAGS_EXEC	?
					PE32_SCN_CNT_CODE		:
				segment->filesize == 0			?
					PE32_SCN_CNT_UNINITIALIZED_DATA	:
					PE32_SCN_CNT_INITIALIZED_DATA,
		};

		memcpy(sheader.name, segname, sizeof(sheader.name));

		sheader.characteristics |=
			(segment->flags & SEGMENT_FLAGS_EXEC	?
				PE32_SCN_MEM_EXECUTE : 0)		|
			(segment->flags & SEGMENT_FLAGS_READ	?
				PE32_SCN_MEM_READ : 0)			|
			(segment->flags & SEGMENT_FLAGS_WRITE	?
				PE32_SCN_MEM_WRITE : 0);

		write_chunk(out, &sheader, sizeof(sheader));
	}

	write_chunk(out, &(PE32Plus_Section_Header) {
		.name			= ".reloc",
		.virtualSize		= peRelocs->size,
		.virtualAddress		= relocVaddr - baseAddress,
		.sizeOfRawData		= lastLayout->size,
		.pointerToRawData	= lastLayout->offset,
		.pointerToRelocations	= 0,
		.pointerToLineNumbers	= 0,
		.numberOfRelocations	= 0,
		.numberOfLineNumbers	= 0,
		.characteristics	= PE32_SCN_CNT_INITIALIZED_DATA	|
					  PE32_SCN_MEM_DISCARDABLE	|
					  PE32_SCN_MEM_READ,
	}, sizeof(PE32Plus_Section_Header));

	write_padding(out, sizeOfHeaders -
			   PE32Plus_Image_Header_Size(exec->segnum + 1));

	for (size_t i = 0; i < exec->segnum; i++) {
		Segment *segment = &exec->segments[i];

		write_chunk(out, segment->data.rawData, segment->filesize);
		write_padding(out, layouts[i].size - segment->filesize);
	}

	write_chunk(out, peRelocs->rawData, peRelocs->size);
	write_padding(out, lastLayout->size - peRelocs->size);

	free(layouts);

	fclose(out);
}

static void
help(char *argv[])
{
	fprintf(stderr, "Usage: %s [OPTIONS] <ELF_INPUT> <PE_OUTPUT>\n",
		argv[0]);
	fprintf(stderr, "\nConvert ELF executable to PE files suitable for "
			"using as EFI applications\n\n");
	fprintf(stderr, "\t-b	Set PE base address\n");
	fprintf(stderr, "\t-h	Print this help\n");
	fprintf(stderr, "\t-v	Increase verbosity\n");
}

int
main(int argc, char *argv[])
{
	uint64_t baseAddress = 0x10000000;

	char *endp = NULL;
	char optc;
	while ((optc = getopt(argc, argv, "b:hv")) > 0) {
		switch (optc) {
		case 'b':
			endp = NULL;
			baseAddress = strtoul(optarg, &endp, 0);
			assert_msg(!*endp, "invalid base address \"%s\"\n",
				   optarg);
			break;
		case 'v':
			gVerbosity++;
			break;
		case 'h':
			help(argv);
			exit(0);
			break;
		case ':': // fallthrough
		case '?':
			help(argv);
			exit(1);
			break;
		}
	}

	if (optind != argc - 2) {
		help(argv);
		exit(1);
	}

	Sized_Data elf = load_binary_file(argv[optind]);

	Executable exec = parse_elf(&elf);
	free_sized_data(&elf);

	relocate_exec(&exec, baseAddress);

	apply_relocs(&exec);

	Sized_Data peRelocs = create_pe_reloc(&exec, baseAddress);

	write_pe(argv[optind + 1], &exec, baseAddress, &peRelocs);

	free_sized_data(&peRelocs);

	for (size_t i = 0; i < exec.segnum; i++)
		free_sized_data(&exec.segments[i].data);
	free(exec.segments);
	free(exec.relocs);

	free_sized_data(&exec.dynamic.data);

	return 0;
}
