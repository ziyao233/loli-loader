-- SPDX-License-Identifier: MPL-2.0
--[[
--	loli-loader testsuite
--	/tests/extlinux.lua
--	Copyright (c) 2024 Yao Zi.
--]]

local io		= require "io";
local string		= require "string";
local os		= require "os";

local extlinux		= require "lextlinux";

local verboseLevel	= os.getenv("VERBOSE");

local function
verbose(fmt, ...)
	if verboseLevel then
		print(" ===> " .. string.format(fmt, ...));
	end
end

local function
get(iter, self, key, expect)
	local ret = extlinux.get(iter, key);
	verbose("get key %q", key);
	assert(ret == expect,
	       ("expect %q, got %q"):format(expect, ret));
	return iter;
end

local function
next(iter, self, expect)
	iter = extlinux.next(iter, key);

	verbose("find next entry, expect %s", expect and expect or "not found");

	if not expect then
		assert(not iter);
		return nil;
	end

	assert(iter);

	assert(extlinux.get(iter, "label") == expect,
	       ("expect label to be %q, got %q"):format(expect, ret));
	return iter;
end

local cases = {
	{
		name	= "example",
		conf	= [[
timeout 10
label Linux 6.6
	kernel  /Image-6.6
	initrd  /initramfs-6.6.img
	append  console=tty0 quiet
label Linux 6.12
	kernel  /Image-6.12
	initrd  /initramfs-6.12.img
	append  console=tty0 quiet
]],
		opts	= {
			{ get, "timeout", "10" },
			{ get, "default", nil },
			{ next, "Linux 6.6" },
			{ get, "kernel", "/Image-6.6" },
		},
	},
	{
		name	= "no entries",
		conf	= [[
timeout 10
default 1

]],
		opts	= {
			{ get, "timeout", "10" },
			{ get, "default", "1" },
			{ next, nil },
		},
	},
	{
		name	= "no global options",
		conf	= [[
label test1
	kernel image1
label test2
	kernel image2
]],
		opts	= {
			{ get, "timeout", nil },
			{ get, "default", nil },
			{ next, "test1" },
			{ get, "kernel", "image1" },
			{ next, "test2" },
			{ get, "kernel", "image2" },
			{ next, nil },
		},
	},
	{
		--[[
		--	We consider pairs without a value malformed, but they
		--	shouldn't break parsing on other lines
		--]]
		name	= "pairs without value",
		conf	= [[
timeout 10
default 1
empty
empty2	
quiet 0
label test
	empty
	append quiet
]],
		opts	= {
			{ get, "timeout", "10" },
			{ get, "empty", nil },
			{ get, "empty2", nil },
			{ get, "quiet", "0" },
			{ next, "test", "0" },
			{ get, "empty", nil },
			{ get, "append", "quiet" },
			{ next, nil },
		},
	},
	{
		name	= "missing newline at EOF",
		conf	= [[
label test1
kernel test1]],
		opts	= {
			{ next, "test1" },
			{ get, "kernel", "test1" },
			{ next, nil },
		},
	}
};

local function
runCase(conf, opts)
	local iter = conf;

	for _, opt in ipairs(opts) do
		iter = opt[1](iter, table.unpack(opt));
	end
end

for i, case in ipairs(cases) do
	io.stdout:write(("case %d: %s "):format(i, case.name));
	io.stdout:flush();

	runCase(case.conf, case.opts);

	io.stdout:write("[OK]\n");
end
