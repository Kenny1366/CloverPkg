/*
Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <Library/Platform/Platform.h>
#include <Library/Platform/DeviceTree.h>
#include <Library/Platform/KernelPatcher.h>

#define PATCH_DEBUG 0
#define MEM_DEB 0

#if PATCH_DEBUG
#define DBG(...)  Print(__VA_ARGS__);
#else
#define DBG(...)
#endif

EFI_EVENT   ExitBootServiceEvent = NULL;
#if 0
EFI_EVENT   mVirtualAddressChangeEvent = NULL;
EFI_EVENT   OnReadyToBootEvent = NULL;
EFI_EVENT   mSimpleFileSystemChangeEvent = NULL;
#endif

VOID
EFIAPI
OnExitBootServices (IN EFI_EVENT Event, IN VOID *Context) {
  //EFI_STATUS Status;
  /*
  if (DoHibernateWake) {
    gST->ConOut->OutputString (gST->ConOut, L"wake!!!");
    gBS->Stall(5000000);     // 5 seconds delay
    return;
  }
  */

  //Print(L"OnExitBootServices.....+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  gST->ConOut->OutputString (gST->ConOut, L"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  //
  // Patch kernel and kexts if needed
  //
  KernelAndKextsPatcherStart((LOADER_ENTRY *)Context);
}

#if 0
VOID
EFIAPI
OnReadyToBoot (
  IN EFI_EVENT    Event,
  IN VOID         *Context
) {
//
}

VOID
EFIAPI
VirtualAddressChangeEvent (
  IN EFI_EVENT    Event,
  IN VOID         *Context
) {
//  EfiConvertPointer (0x0, (VOID **) &mProperty);
//  EfiConvertPointer (0x0, (VOID **) &mSmmCommunication);
}

VOID
EFIAPI
OnSimpleFileSystem (
  IN EFI_EVENT    Event,
  IN VOID         *Context
) {
  EFI_TPL   OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  gEvent = 1;
  //ReinitRefitLib();
  //ScanVolumes();
  //enter GUI
  // DrawMenuText(L"OnSimpleFileSystem", 0, 0, UGAHeight-40, 1);
  // MsgLog("OnSimpleFileSystem occured\n");

  gBS->RestoreTPL (OldTpl);
}

EFI_STATUS
GuiEventsInitialize () {
  EFI_STATUS      Status;
  EFI_EVENT       Event;
  VOID            *RegSimpleFileSystem = NULL;

  gEvent = 0;
  Status = gBS->CreateEvent (
                 EVT_NOTIFY_SIGNAL,
                 TPL_NOTIFY,
                 OnSimpleFileSystem,
                 NULL,
                 &Event
                );

  if (!EFI_ERROR(Status)) {
    Status = gBS->RegisterProtocolNotify (
                    &gEfiSimpleFileSystemProtocolGuid,
                    Event,
                    &RegSimpleFileSystem
                  );
  }


  return Status;
}
#endif

EFI_STATUS
EventsInitialize (
  IN LOADER_ENTRY   *Entry
) {
  EFI_STATUS      Status;
  VOID*           Registration = NULL;

  //
  // Register the event to reclaim variable for OS usage.
  //
  //EfiCreateEventReadyToBoot(&OnReadyToBootEvent);
  /*
  EfiCreateEventReadyToBootEx (
   TPL_NOTIFY,
   OnReadyToBoot,
   NULL,
   &OnReadyToBootEvent
   );
  */

  //
  // Register notify for exit boot services
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  OnExitBootServices,
                  Entry,
                  &ExitBootServiceEvent
                );

  if(!EFI_ERROR(Status)) {
    /*Status = */gBS->RegisterProtocolNotify (
                       &gEfiStatusCodeRuntimeProtocolGuid,
                       ExitBootServiceEvent,
                       &Registration
                     );
  }

  //
  // Register the event to convert the pointer for runtime.
  //
  /*
  gBS->CreateEventEx (
        EVT_NOTIFY_SIGNAL,
        TPL_NOTIFY,
        VirtualAddressChangeEvent,
        NULL,
        &gEfiEventVirtualAddressChangeGuid,
        &mVirtualAddressChangeEvent
      );
  */

  return EFI_SUCCESS;
}

EFI_STATUS
EjectVolume (
  IN REFIT_VOLUME   *Volume
) {
  //UINT64                          Lun = 0;
  //UINT8                           *Target;
  //UINT8                           TargetArray[EFI_SCSI_TARGET_MAX_BYTES];
  EFI_SCSI_IO_PROTOCOL              *ScsiIo = NULL;
  EFI_SCSI_IO_SCSI_REQUEST_PACKET   CommandPacket;
  EFI_STATUS                        Status; // = EFI_UNSUPPORTED;
  USB_MASS_DEVICE                   *UsbMass = NULL;
  EFI_BLOCK_IO_PROTOCOL             *BlkIo  = NULL;
  EFI_BLOCK_IO_MEDIA                *Media;
  UINT32                            Timeout, CmdResult;
  UINT8                             Cdb[EFI_SCSI_OP_LENGTH_SIX];

  //
  // Initialize SCSI REQUEST_PACKET and 6-byte Cdb
  //
  ZeroMem (&CommandPacket, sizeof (EFI_SCSI_IO_SCSI_REQUEST_PACKET));
  ZeroMem (Cdb, EFI_SCSI_OP_LENGTH_SIX);

  Status = gBS->HandleProtocol(Volume->DeviceHandle, &gEfiScsiIoProtocolGuid, (VOID **) &ScsiIo);
  if (ScsiIo) {
    //Target = &TargetArray[0];
    //ScsiIo->GetDeviceLocation (ScsiIo, &Target, &Lun);

    Cdb[0]  = EFI_SCSI_OP_START_STOP_UNIT;
    //Cdb[1]  = (UINT8) (LShiftU64 (Lun, 5) & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
    //Cdb[1] |= 0x01;
    Cdb[1]  = 0x01;
    Cdb[4]  = ATA_CMD_SUBOP_EJECT_DISC;
    CommandPacket.Timeout = EFI_TIMER_PERIOD_SECONDS (3);
    CommandPacket.Cdb = Cdb;
    CommandPacket.CdbLength = (UINT8) sizeof (Cdb);

    Status = ScsiIo->ExecuteScsiCommand (ScsiIo, &CommandPacket, NULL);
  } else {
    Status = gBS->HandleProtocol(Volume->DeviceHandle, &gEfiBlockIoProtocolGuid, (VOID **) &BlkIo);
    if (BlkIo) {
      UsbMass = USB_MASS_DEVICE_FROM_BLOCK_IO (BlkIo);
      if (!UsbMass) {
        MsgLog("no UsbMass\n");
        Status = EFI_NOT_FOUND;
        goto ON_EXIT;
      }

      Media   = &UsbMass->BlockIoMedia;
      if (!Media) {
        MsgLog("no BlockIoMedia\n");
        Status = EFI_NO_MEDIA;
        goto ON_EXIT;
      }

      //
      // If it is a removable media, such as CD-Rom or Usb-Floppy,
      // need to detect the media before each read/write. While some of
      // Usb-Flash is marked as removable media.
      //
      //TODO - DetectMedia will appear automatically. Do nothing?
      if (!Media->RemovableMedia) {
        //Status = UsbBootDetectMedia (UsbMass);
        //if (EFI_ERROR (Status)) {
          Status = EFI_UNSUPPORTED;
          goto ON_EXIT;
        //}
      }

      if (!(Media->MediaPresent)) {
        Status = EFI_NO_MEDIA;
        goto ON_EXIT;
      }

      //TODO - remember previous state
      /*
      if (MediaId != Media->MediaId) {
       Status = EFI_MEDIA_CHANGED;
       goto ON_EXIT;
       }
      */

      Timeout = USB_BOOT_GENERAL_CMD_TIMEOUT;
      Cdb[0]  = EFI_SCSI_OP_START_STOP_UNIT;
      //Cdb[1]  = (UINT8) (USB_BOOT_LUN(UsbMass->Lun) & EFI_SCSI_LOGICAL_UNIT_NUMBER_MASK);
      //Cdb[1] |= 0x01;
      Cdb[1] = 0x01;
      Cdb[4] = ATA_CMD_SUBOP_EJECT_DISC; //eject command.
      //Status = EFI_UNSUPPORTED;
      Status    = UsbMass->Transport->ExecCommand (
                                         UsbMass->Context,
                                         &Cdb,
                                         sizeof(Cdb),
                                         EfiUsbDataOut,
                                         NULL, 0,
                                         UsbMass->Lun,
                                         Timeout,
                                         &CmdResult
                                       );

      //ON_EXIT:
      //  gBS->RestoreTPL (OldTpl);
    }
  }

ON_EXIT:

  return Status;
}