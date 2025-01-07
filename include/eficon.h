// SPDX-License-Identifer: MPL-2.0
/*
 *	loli-loader
 *	/include/eficon.h
 */

#ifndef __LOLI_EFICON_H_INC__
#define __LOLI_EFICON_H_INC__

#include <efidef.h>

#pragma pack(push, 0)

typedef struct {
	uint16_t scanCode;
	wchar_t unicodeChar;
} Efi_Input_Key;

typedef struct Efi_Simple_Text_Input_Protocol {
	Efi_Status (*reset)(struct Efi_Simple_Text_Input_Protocol *p,
			    bool extendedVerification);
	Efi_Status (*readKeyStroke)(struct Efi_Simple_Text_Input_Protocol *p,
				    Efi_Input_Key *key);
	Efi_Event waitForKey;
} Efi_Simple_Text_Input_Protocol;

typedef struct Efi_Simple_Text_Output_Protocol {
        Efi_Status (*reset)(struct Efi_Simple_Text_Output_Protocol *p,
			    bool extendedVerifcation);
	Efi_Status (*outputString)(struct Efi_Simple_Text_Output_Protocol *p,
				   wchar_t *str);
} Efi_Simple_Text_Output_Protocol;

#pragma pack(pop)

#endif // __LOLI_EFICON_H_INC__
