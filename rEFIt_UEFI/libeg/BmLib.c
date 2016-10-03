/** @file
  Utility routines used by boot maintenance modules.

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Platform.h"

/**

  Find the first instance of this Protocol
  in the system and return it's interface.


  @param ProtocolGuid    Provides the protocol to search for
  @param Interface       On return, a pointer to the first interface
                         that matches ProtocolGuid

  @retval  EFI_SUCCESS      A protocol instance matching ProtocolGuid was found
  @retval  EFI_NOT_FOUND    No protocol instances were found that match ProtocolGuid

**/
EFI_STATUS
EfiLibLocateProtocol (
  IN  EFI_GUID    *ProtocolGuid,
  OUT VOID        **Interface
) {
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
    ProtocolGuid,
    NULL,
    (VOID **) Interface
  );

  return Status;
}

/**

  Function opens and returns a file handle to the root directory of a volume.

  @param DeviceHandle    A handle for a device

  @return A valid file handle or NULL is returned

**/
EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE   DeviceHandle
) {
  EFI_STATUS                        Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Volume;
  EFI_FILE_HANDLE                   File;

  File = NULL;

  //
  // File the file system interface to the device
  //
  Status = gBS->HandleProtocol (
    DeviceHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &Volume
  );

  //
  // Open the root directory of the volume
  //
  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (
      Volume,
      &File
    );
  }

  return EFI_ERROR (Status) ? NULL : File;
}

/**

  Function gets the file system information from an open file descriptor,
  and stores it in a buffer allocated from pool.


  @param FHand           The file handle.

  @return                A pointer to a buffer with file information.
  @retval                NULL is returned if failed to get Volume Label Info.

**/
EFI_FILE_SYSTEM_VOLUME_LABEL
*EfiLibFileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE    FHand
) {
  EFI_STATUS    Status;
  UINTN         Size = 0;
  EFI_FILE_SYSTEM_VOLUME_LABEL *VolumeInfo = NULL;

  Status = FHand->GetInfo (FHand, &gEfiFileSystemVolumeLabelInfoIdGuid, &Size, VolumeInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // inc size by 2 because some drivers (HFSPlus.efi) do not count 0 at the end of file name
    Size += 2;
    VolumeInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileSystemVolumeLabelInfoIdGuid, &Size, VolumeInfo);
    // Check to make sure this isn't actually EFI_FILE_SYSTEM_INFO
    if (!EFI_ERROR(Status)) {
       EFI_FILE_SYSTEM_INFO *FSInfo = (EFI_FILE_SYSTEM_INFO *)VolumeInfo;
       if (FSInfo->Size == (UINT64)Size) {
          // Allocate a new volume label
          VolumeInfo = (EFI_FILE_SYSTEM_VOLUME_LABEL *)EfiStrDuplicate(FSInfo->VolumeLabel);
          FreePool(FSInfo);
       }
    }

    if (!EFI_ERROR(Status)) {
       return VolumeInfo;
    }

    FreePool(VolumeInfo);
  }

  return NULL;
}

/**
  Duplicate a string.

  @param Src             The source.

  @return A new string which is duplicated copy of the source.
  @retval NULL If there is not enough memory.

**/
CHAR16
*EfiStrDuplicate (
  IN CHAR16   *Src
) {
  CHAR16  *Dest;
  UINTN   Size;

  Size  = StrSize (Src); //at least 2bytes
  Dest  = AllocatePool (Size);
  //ASSERT (Dest != NULL);
  if (Dest != NULL) {
    CopyMem (Dest, Src, Size);
  }

  return Dest;
}

//Compare strings case insensitive
INTN
StriCmp (
  IN  CONST CHAR16   *FirstS,
  IN  CONST CHAR16   *SecondS
) {
  if (
    (FirstS == NULL) || (SecondS == NULL) ||
    (StrLen(FirstS) != StrLen(SecondS))
  ) {
    return 1;
  }

  while (*FirstS != L'\0') {
    if ( (((*FirstS >= 'a') && (*FirstS <= 'z')) ? (*FirstS - ('a' - 'A')) : *FirstS ) !=
      (((*SecondS >= 'a') && (*SecondS <= 'z')) ? (*SecondS - ('a' - 'A')) : *SecondS ) ) break;
    FirstS++;
    SecondS++;
  }

  return *FirstS - *SecondS;
}

// If Null-terminated strings are case insensitive equal or its sSize symbols are equal then TRUE
BOOLEAN
AsciiStriNCmp (
  IN  CONST CHAR8   *FirstS,
  IN  CONST CHAR8   *SecondS,
  IN  CONST UINTN    sSize
) {
  INTN    i = sSize;

  while ( i && (*FirstS != '\0') ) {
    if ( (((*FirstS >= 'a') && (*FirstS <= 'z')) ? (*FirstS - ('a' - 'A')) : *FirstS ) !=
      (((*SecondS >= 'a') && (*SecondS <= 'z')) ? (*SecondS - ('a' - 'A')) : *SecondS ) ) return FALSE;
    FirstS++;
    SecondS++;
    i--;
  }

  return TRUE;
}

// Case insensitive search of WhatString in WhereString
BOOLEAN
AsciiStrStriN (
  IN  CONST CHAR8   *WhatString,
  IN  CONST UINTN   sWhatSize,
  IN  CONST CHAR8   *WhereString,
  IN  CONST UINTN   sWhereSize
) {
  INTN      i = sWhereSize;
  BOOLEAN   Found = FALSE;

  if (sWhatSize > sWhereSize) return FALSE;
  for (; i && !Found; i--) {
    Found = AsciiStriNCmp(WhatString, WhereString, sWhatSize);
    WhereString++;
  }

  return Found;
}

/**

  Function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

  @param FHand           File Handle.

  @return                A pointer to a buffer with file information or NULL is returned

**/
EFI_FILE_INFO
*EfiLibFileInfo (
  IN EFI_FILE_HANDLE    FHand
) {
  EFI_STATUS      Status;
  EFI_FILE_INFO   *FileInfo = NULL;
  UINTN           Size = 0;

  Status = FHand->GetInfo (FHand, &gEfiFileInfoGuid, &Size, FileInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // inc size by 2 because some drivers (HFSPlus.efi) do not count 0 at the end of file name
    Size += 2;
    FileInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileInfoGuid, &Size, FileInfo);
  }

  return EFI_ERROR(Status)?NULL:FileInfo;
}

EFI_FILE_SYSTEM_INFO
*EfiLibFileSystemInfo (
  IN EFI_FILE_HANDLE    FHand
) {
  EFI_STATUS    Status;
  UINTN         Size = 0;
  EFI_FILE_SYSTEM_INFO    *FileSystemInfo = NULL;

  Status = FHand->GetInfo (FHand, &gEfiFileSystemInfoGuid, &Size, FileSystemInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // inc size by 2 because some drivers (HFSPlus.efi) do not count 0 at the end of file name
    Size += 2;
    FileSystemInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileSystemInfoGuid, &Size, FileSystemInfo);
  }

  return EFI_ERROR(Status)?NULL:FileSystemInfo;
}

/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
) {
  UINTN   Count = 0, Size;

  while (GetNextDevicePathInstance (&DevicePath, &Size) != NULL) {
    Count += 1;
  }

  return Count;
}

/**
  Adjusts the size of a previously allocated buffer.


  @param OldPool         - A pointer to the buffer whose size is being adjusted.
  @param OldSize         - The size of the current buffer.
  @param NewSize         - The size of the new buffer.

  @return   The newly allocated buffer.
  @retval   NULL  Allocation failed.

**/
VOID
*EfiReallocatePool (
  IN VOID     *OldPool,
  IN UINTN    OldSize,
  IN UINTN    NewSize
) {
  VOID  *NewPool = NULL;

  if (NewSize != 0) {
    NewPool = AllocateZeroPool (NewSize);
  }

  if (OldPool != NULL) {
    if (NewPool != NULL) {
      CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
    }

    FreePool (OldPool);
  }

  return NewPool;
}

INTN
TimeCompare (
  IN EFI_TIME   *Time1,
  IN EFI_TIME   *Time2
) {
 INTN Comparison;

 if (Time1 == NULL) {
   if (Time2 == NULL) {
     return 0;
   } else {
     return -1;
   }
 } else if (Time2 == NULL) {
   return 1;
 }

 Comparison = Time1->Year - Time2->Year;
 if (Comparison == 0) {
   Comparison = Time1->Month - Time2->Month;
   if (Comparison == 0) {
     Comparison = Time1->Day - Time2->Day;
     if (Comparison == 0) {
       Comparison = Time1->Hour - Time2->Hour;
       if (Comparison == 0) {
         Comparison = Time1->Minute - Time2->Minute;
         if (Comparison == 0) {
           Comparison = Time1->Second - Time2->Second;
           if (Comparison == 0) {
             Comparison = Time1->Nanosecond - Time2->Nanosecond;
           }
         }
       }
     }
   }
 }

 return Comparison;
}

