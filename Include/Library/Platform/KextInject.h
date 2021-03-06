/*
kext injection
*/

#ifndef __KEXT_INJECT_H__
#define __KEXT_INJECT_H__

#include "KernelPatcher.h"

////////////////////
// defines
////////////////////
#define RoundPage(x)  ((((UINTN)(x)) + EFI_PAGE_SIZE - 1) & ~(EFI_PAGE_SIZE - 1))

#define KEXT_SIGNATURE  SIGNATURE_32('M','O','S','X')

/*
 * Capability bits used in the definition of cpu_type.
 */
#define CPU_ARCH_MASK   0xff000000    /* mask for architecture bits */
#define CPU_ARCH_ABI64  0x01000000    /* 64 bit ABI */

/*
 *  Machine types known by all.
 */

#define CPU_TYPE_ANY      ((cpu_type_t) -1)
#define CPU_TYPE_VAX      ((cpu_type_t) 1)
#define CPU_TYPE_X86      ((cpu_type_t) 7)
#define CPU_TYPE_X86_64   (CPU_TYPE_X86 | CPU_ARCH_ABI64)

#define FAT_MAGIC 0xcafebabe
#define FAT_CIGAM 0xbebafeca  /* NXSwapLong(FAT_MAGIC) */

#define THIN_IA32 0xfeedface
#define THIN_X64  0xfeedfacf

// Chameleon: Micky1979
#define IOPERSONALITIES_INJECTOR_PLIST_TPL  L"<\?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\
<plist version=\"1.0\">\n\
<dict>\n\
  <key>CFBundleDevelopmentRegion</key>\n\
  <string>English</string>\n\
  <key>CFBundleIdentifier</key>\n\
  <string>com.Enoch.plist.%a</string>\n\
  <key>CFBundleInfoDictionaryVersion</key>\n\
  <string>6.0</string>\n\
  <key>CFBundleName</key>\n\
  <string>%a</string>\n\
  <key>CFBundlePackageType</key>\n\
  <string>KEXT</string>\n\
  <key>CFBundleSignature</key>\n\
  <string>\?\?\?\?</string>\n\
  <key>CFBundleVersion</key>\n\
  <string>1.0.0</string>\n\
  <key>IOKitPersonalities</key>\n\
  <dict>\n%s\
  </dict>\n%s\
</dict>\n\
</plist>"

#define IOPERSONALITIES_INJECTOR_OSBUNDLEREQUIRED_TPL L"  <key>OSBundleRequired</key>\n  <string>%a</string>\n"

////////////////////
// types
////////////////////
typedef struct {
  UINT32   magic;         /* FAT_MAGIC */
  UINT32   nfat_arch;     /* number of structs that follow */
} FAT_HEADER;

typedef struct {
  UINT32  cputype;        /* cpu specifier (int) */
  UINT32  cpusubtype;     /* machine specifier (int) */
  UINT32  offset;         /* file offset to this object file */
  UINT32  size;           /* size of this object file */
  UINT32  align;          /* alignment as a power of 2 */
} FAT_ARCH;

typedef struct {
  UINT32              Signature;
  LIST_ENTRY          Link;
  DeviceTreeBuffer    Kext;
} KEXT_ENTRY;

////////////////////
// functions
////////////////////
EFI_STATUS
LoadKexts (
  IN LOADER_ENTRY   *Entry
);

EFI_STATUS
InjectKexts (
  IN UINT32         deviceTreeP,
  IN UINT32         *deviceTreeLength,
  IN LOADER_ENTRY   *Entry
);

EFI_STATUS
EFIAPI
ThinFatFile (
  IN OUT  UINT8         **Binary,
  IN OUT  UINTN         *Length,
  IN      cpu_type_t    ArchCpuType
);

#endif
