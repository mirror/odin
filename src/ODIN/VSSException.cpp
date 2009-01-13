/******************************************************************************

    ODIN - Open Disk Imager in a Nutshell

    Copyright (C) 2008

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>

    For more information and the latest version of the source code see
    <http://sourceforge.net/projects/odin-win>

******************************************************************************/
 
#include "stdafx.h"
#include "vsserror.h"
#include "VSSException.h"

#ifdef DEBUG
  #define new DEBUG_NEW
  #define malloc DEBUG_MALLOC
#endif // _DEBUG

using namespace std;

// error message strings (should later be moved to a separate non compiled file)
LPCWSTR EVSSException::sMessages[] = {
  L"no error",                                  // noCode not to be used
  L"Error in COM function",                     // comError          
  L"Error in Volume Shadow Copy Service: {0}",  //vssError
};

EVSSException::ErrorMessage EVSSException::sVSSErrorMessages[] = {
  { VSS_E_BAD_STATE, L"A function call was made when the object was in an incorrect state for that function." }, 
  { VSS_E_PROVIDER_ALREADY_REGISTERED, L"The provider has already been registered." },
  { VSS_E_PROVIDER_NOT_REGISTERED, L"The volume shadow copy provider is not registered in the system." },
  { VSS_E_PROVIDER_VETO, L"The shadow copy provider had an error. Please see the system and application event logs for more information." },
  { VSS_E_PROVIDER_IN_USE, L"The shadow copy provider is currently in use and cannot be unregistered." },
  { VSS_E_OBJECT_NOT_FOUND, L"The specified object was not found." },
  { VSS_S_ASYNC_PENDING, L"The asynchronous operation is pending." },
  { VSS_S_ASYNC_FINISHED, L"The asynchronous operation has completed." },
  { VSS_S_ASYNC_CANCELLED, L"The asynchronous operation has been cancelled." },
  { VSS_E_VOLUME_NOT_SUPPORTED, L"Shadow copying the specified volume is not supported." },
  { VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER, L"The given shadow copy provider does not support shadow copying the specified volume." },
  { VSS_E_OBJECT_ALREADY_EXISTS, L"The object already exists." },
  { VSS_E_UNEXPECTED_PROVIDER_ERROR, L"The shadow copy provider had an unexpected error while trying to process the specified operation." },
  { VSS_E_CORRUPT_XML_DOCUMENT, L"The given XML document is invalid. It is either incorrectly-formed XML or it does not match the schema. This error code is deprecated." },
  { VSS_E_INVALID_XML_DOCUMENT, L"The given XML document is invalid. It is either incorrectly-formed XML or it does not match the schema." },
  { VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED, L"The maximum number of volumes for this operation has been rached." },
  { VSS_E_FLUSH_WRITES_TIMEOUT, L"The shadow copy provider timed out while flushing data to the volume being shadow copied. This is probably due to excessive activity on the volume. Try again later when the volume is not being used so heavily." },
  { VSS_E_HOLD_WRITES_TIMEOUT, L"The shadow copy provider timed out while holding writes to the volume being shadow copied. This is probably due to excessive activity on the volume by an application or a system service. Try again later when activity on the volume is reduced." },
  { VSS_E_UNEXPECTED_WRITER_ERROR, L"VSS encountered problems while sending events to writers." },
  { VSS_E_SNAPSHOT_SET_IN_PROGRESS, L"Another shadow copy creation is already in progress. Please wait a few moments and try again." },
  { VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED, L"The specified volume has already reached its maximum number of shadow copies." },
  { VSS_E_WRITER_INFRASTRUCTURE, L"An error was detected in the Volume Shadow Copy Service (VSS). The problem occurred while trying to contact VSS writers." },
  { VSS_E_WRITER_NOT_RESPONDING, L"A writer did not respond to a GatherWriterStatus call. The writer may either have terminated or it may be stuck." },
  { VSS_E_WRITER_ALREADY_SUBSCRIBED, L"The writer has already sucessfully called the Subscribe function. It cannot call subscribe multiple times." },
  { VSS_E_UNSUPPORTED_CONTEXT, L"The shadow copy provider does not support the specified shadow copy type." },
  { VSS_E_VOLUME_IN_USE, L"The specified shadow copy storage association is in use and so can't be deleted." },
  { VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED, L"Maximum number of shadow copy storage associations already reached." },
  { VSS_E_INSUFFICIENT_STORAGE, L"Insufficient storage available to create either the shadow copy storage file or other shadow copy data." },
  { VSS_E_NO_SNAPSHOTS_IMPORTED, L"No shadow copies were successfully imported." },
  { VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED, L"Some shadow copies were not succesfully imported." },
  { VSS_E_SOME_SNAPSHOTS_NOT_IMPORTED, L"Some shadow copies were not succesfully imported." },
  { VSS_E_MAXIMUM_NUMBER_OF_REMOTE_MACHINES_REACHED, L"The maximum number of remote machines for this operation has been reached." },
  { VSS_E_REMOTE_SERVER_UNAVAILABLE, L"The remote server is unavailable." },
  { VSS_E_REMOTE_SERVER_UNSUPPORTED, L"The remote server is running a version of the Volume Shadow Copy Service that does not support remote shadow-copy creation." },
  { VSS_E_REVERT_IN_PROGRESS, L"A revert is currently in progress for the specified volume. Another revert cannot be initiated until the current revert completes." },
  { VSS_E_REVERT_VOLUME_LOST, L"The volume being reverted was lost during revert." },
  { VSS_E_REBOOT_REQUIRED, L"A reboot is required after completing this operation." },
  { VSS_E_TRANSACTION_FREEZE_TIMEOUT, L"A timeout occured while freezing a transaction manager." },
  { VSS_E_TRANSACTION_THAW_TIMEOUT, L"Too much time elapsed between freezing a transaction manager and thawing  the transaction manager." },  
  { VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT, L"the shadow-copy set only contains only a subset of the volumes needed to correctly backup the selected components of the writer." },
  { VSS_E_WRITERERROR_OUTOFRESOURCES, L"A resource allocation failed while processing this operation." },
  { VSS_E_WRITERERROR_TIMEOUT, L"The writer's timeout expired between the Freeze and Thaw events." },
  { VSS_E_WRITERERROR_RETRYABLE, L"The writer experienced a transient error. If the backup process is retried, the error may not reoccur." },
  { VSS_E_WRITERERROR_NONRETRYABLE, L"The writer experienced a non-transient error. If the backup process is retried, the error is likely to reoccur." },
  { VSS_E_WRITERERROR_RECOVERY_FAILED, L"The writer experienced an error while trying to recover the shadow-copy volume." },
  { VSS_E_ASRERROR_DISK_ASSIGNMENT_FAILED, L"There are too few disks on this computer or one or more of the disks is too small. Add or change disks so they match the disks in the backup, and try the restore again." },
  { VSS_E_ASRERROR_DISK_RECREATION_FAILED, L"Windows cannot create a disk on this computer needed to restore from the backup. Make sure the disks are properly connected, or add or change disks, and try the restore again." }
};

const wchar_t* EVSSException::GetVSSMessageForErrorCode() {
  for (int i=0; i<sizeof(sVSSErrorMessages)/sizeof(sVSSErrorMessages[0]); i++) {
    if (fErrorCode = sVSSErrorMessages[i].code)
      return sVSSErrorMessages[i].msg;
  }
  return NULL;
}

void EVSSException::AppendVSSMessage() {
  const wchar_t* message = GetVSSMessageForErrorCode();

  if (message)
    fMessage += message;
  else
    fMessage += L"Unknown VSS Error";

}
