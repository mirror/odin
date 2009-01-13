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

#pragma once
#ifndef _VSBACKUP_XP_H_
#define _VSBACKUP_XP_H_

// Important note
// Microsoft has changed the interface from WIndows XP to subsequent versions without changing the interface names
// The official way is to provide different binaries for XP and 2003 and  beyond
// We avoid this by cheching the platfrom at runtime and changing the interface and structure names


// description of a component
typedef struct _VSS_COMPONENTINFO_XP
	{
	VSS_COMPONENT_TYPE type;	// either VSS_CT_DATABASE or VSS_CT_FILEGROUP
	BSTR bstrLogicalPath;		// logical path to component
	BSTR bstrComponentName;		// component name
	BSTR bstrCaption;		// description of component
	BYTE *pbIcon;			// icon
	UINT cbIcon;			// icon
	bool bRestoreMetadata;		// whether component supplies restore metadata
	bool bNotifyOnBackupComplete;	// whether component needs to be informed if backup was successful
	bool bSelectable;		// is component selectable	
	UINT cFileCount;		// # of files in file group
	UINT cDatabases;		// # of database files
	UINT cLogFiles;			// # of log files
} VSS_COMPONENTINFO_XP;

typedef const VSS_COMPONENTINFO_XP *PVSSCOMPONENTINFO_XP;


// component information
class IVssWMComponentXP : public IUnknown
	{
public:
	// get component information
	STDMETHOD(GetComponentInfo)(PVSSCOMPONENTINFO *ppInfo) = 0;

	// free component information
	STDMETHOD(FreeComponentInfo)(PVSSCOMPONENTINFO pInfo) = 0;

	// obtain a specific file in a file group
	STDMETHOD(GetFile)
		(
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		) = 0;

	// obtain a specific physical database file for a database
	STDMETHOD(GetDatabaseFile)
		(
		IN UINT iDBFile,
		OUT IVssWMFiledesc **ppFiledesc
		) = 0;

	// obtain a specific physical log file for a database
	STDMETHOD(GetDatabaseLogFile)
		(
		IN UINT iDbLogFile,
		OUT IVssWMFiledesc **ppFiledesc
		) = 0;
	};


// interface to examine writer metadata
class IVssExamineWriterMetadataXP : public IUnknown
	{
public:
	// obtain identity of the writer
	STDMETHOD(GetIdentity)
		(
		OUT VSS_ID *pidInstance,
		OUT VSS_ID *pidWriter,
		OUT BSTR *pbstrWriterName,
		OUT VSS_USAGE_TYPE *pUsage,
		OUT VSS_SOURCE_TYPE *pSource
		) = 0;

	// obtain number of include files, exclude files, and components
	STDMETHOD(GetFileCounts)
		(
		OUT UINT *pcIncludeFiles,
		OUT UINT *pcExcludeFiles,
		OUT UINT *pcComponents
		) = 0;

	// obtain specific include files
	STDMETHOD(GetIncludeFile)
		(
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		) = 0;

	// obtain specific exclude files
	STDMETHOD(GetExcludeFile)
		(
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		) = 0;

	// obtain specific component
	STDMETHOD(GetComponent)
		(
		IN UINT iComponent,
		OUT IVssWMComponent **ppComponent
		) = 0;

	// obtain restoration method
	STDMETHOD(GetRestoreMethod)
		(
		OUT VSS_RESTOREMETHOD_ENUM *pMethod,
		OUT BSTR *pbstrService,
		OUT BSTR *pbstrUserProcedure,
		OUT VSS_WRITERRESTORE_ENUM *pwriterRestore,
		OUT bool *pbRebootRequired,
		UINT *pcMappings
		) = 0;

	// obtain a specific alternative location mapping
	STDMETHOD(GetAlternateLocationMapping)
		(
		IN UINT iMapping,
		OUT IVssWMFiledesc **ppFiledesc
		) = 0;

	// obtain reference to actual XML document
	STDMETHOD(GetDocument)(IXMLDOMDocument **pDoc) = 0;

	// convert document to a XML string
	STDMETHOD(SaveAsXML)(BSTR *pbstrXML) = 0;

	// load document from an XML string
	STDMETHOD(LoadFromXML)(BSTR bstrXML) = 0;
	};


class IVssWriterComponentsExtXP :
	public IVssWriterComponents,
	public IUnknown
	{
	};


// backup components interface
class IVssBackupComponentsXP : public IUnknown
	{
public:
	// get count of writer components
	STDMETHOD(GetWriterComponentsCount)(OUT UINT *pcComponents) = 0;

	// obtain a specific writer component
	STDMETHOD(GetWriterComponents)
		(
		IN UINT iWriter,
		OUT IVssWriterComponentsExt **ppWriter
		) = 0;

	// initialize and create BACKUP_COMPONENTS document
	STDMETHOD(InitializeForBackup)(IN BSTR bstrXML = NULL) = 0;

	// set state describing backup
	STDMETHOD(SetBackupState)
		(
		IN bool bSelectComponents,
		IN bool bBackupBootableSystemState,
		IN VSS_BACKUP_TYPE backupType,
		IN bool bPartialFileSupport = false
		) = 0;

	STDMETHOD(InitializeForRestore)(IN BSTR bstrXML) = 0;

	// gather writer metadata
	STDMETHOD(GatherWriterMetadata)
		(
		OUT IVssAsync **pAsync
		) = 0;

	// get count of writers with metadata
	STDMETHOD(GetWriterMetadataCount)
		(
		OUT UINT *pcWriters
		) = 0;

	// get writer metadata for a specific writer
	STDMETHOD(GetWriterMetadata)
		(
		IN UINT iWriter,
		OUT VSS_ID *pidInstance,
		OUT IVssExamineWriterMetadataXP **ppMetadata
		) = 0;

	// free writer metadata
	STDMETHOD(FreeWriterMetadata)() = 0;

	// add a component to the BACKUP_COMPONENTS document
	STDMETHOD(AddComponent)
		(
		IN VSS_ID instanceId,
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName
		) = 0;

	// dispatch PrepareForBackup event to writers
	STDMETHOD(PrepareForBackup)
		(
		OUT IVssAsync **ppAsync
		) = 0;

	// abort the backup
	STDMETHOD(AbortBackup)() = 0;

	// dispatch the Identify event so writers can expose their metadata
	STDMETHOD(GatherWriterStatus)
		(
		OUT IVssAsync **pAsync
		) = 0;


	// get count of writers with status
	STDMETHOD(GetWriterStatusCount)
		(
		OUT UINT *pcWriters
		) = 0;

	STDMETHOD(FreeWriterStatus)() = 0;

	STDMETHOD(GetWriterStatus)
		(
		IN UINT iWriter,
		OUT VSS_ID *pidInstance,
		OUT VSS_ID *pidWriter,
		OUT BSTR *pbstrWriter,
		OUT VSS_WRITER_STATE *pnStatus,
		OUT HRESULT *phResultFailure
		) = 0;

	// indicate whether backup succeeded on a component
	STDMETHOD(SetBackupSucceeded)
		(
		IN VSS_ID instanceId,
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bSucceded
		) = 0;

    // set backup options for the writer
	STDMETHOD(SetBackupOptions)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszBackupOptions
		) = 0;

    // indicate that a given component is selected to be restored
    STDMETHOD(SetSelectedForRestore)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bSelectedForRestore
		) = 0;


    // set restore options for the writer
	STDMETHOD(SetRestoreOptions)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszRestoreOptions
		) = 0;

	// indicate that additional restores will follow
	STDMETHOD(SetAdditionalRestores)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN bool bAdditionalRestores
		) = 0;


    // set the backup stamp that the differential or incremental
	// backup is based on
    STDMETHOD(SetPreviousBackupStamp)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszPreviousBackupStamp
		) = 0;



	// save BACKUP_COMPONENTS document as XML string
	STDMETHOD(SaveAsXML)(BSTR *pbstrXML) = 0;

	// signal BackupComplete event to the writers
	STDMETHOD(BackupComplete)(OUT IVssAsync **ppAsync) = 0;

	// add an alternate mapping on restore
	STDMETHOD(AddAlternativeLocationMapping)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE componentType,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec,
		IN bool bRecursive,
		IN LPCWSTR wszDestination
		) = 0;

    // add a subcomponent to be restored
	STDMETHOD(AddRestoreSubcomponent)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE componentType,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszSubComponentLogicalPath,
		IN LPCWSTR wszSubComponentName,
		IN bool bRepair
		) = 0;

	// requestor indicates whether files were successfully restored
	STDMETHOD(SetFileRestoreStatus)
		(
		IN VSS_ID writerId,
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN VSS_FILE_RESTORE_STATUS status
		) = 0;


	// signal PreRestore event to the writers
	STDMETHOD(PreRestore)(OUT IVssAsync **ppAsync) = 0;

	// signal PostRestore event to the writers
	STDMETHOD(PostRestore)(OUT IVssAsync **ppAsync) = 0;

    // Called to set the context for subsequent snapshot-related operations
    STDMETHOD(SetContext)
		(
        IN LONG lContext
        ) = 0;
    
	// start a snapshot set
	STDMETHOD(StartSnapshotSet)
	    (
	    OUT VSS_ID *pSnapshotSetId
	    ) = 0;

	// add a volume to a snapshot set
	STDMETHOD(AddToSnapshotSet)
		(							
		IN VSS_PWSZ		pwszVolumeName, 			
		IN VSS_ID		ProviderId,
		OUT VSS_ID		*pidSnapshot
		) = 0;												

	// create the snapshot set
	STDMETHOD(DoSnapshotSet)
		(								
		OUT IVssAsync** 	ppAsync 					
		) = 0;

   	STDMETHOD(DeleteSnapshots)
		(							
		IN VSS_ID		SourceObjectId, 		
		IN VSS_OBJECT_TYPE 	eSourceObjectType,		
		IN BOOL			bForceDelete,			
		IN LONG*		plDeletedSnapshots,		
		IN VSS_ID*		pNondeletedSnapshotID	
		) = 0;

    STDMETHOD(ImportSnapshots)
		(
		OUT IVssAsync**		ppAsync
		) = 0;

	STDMETHOD(RemountReadWrite)
		(
		IN VSS_ID SnapshotId,
		OUT IVssAsync**		pAsync
		) = 0;

	STDMETHOD(BreakSnapshotSet)
		(
		IN VSS_ID			SnapshotSetId
		) = 0;

	STDMETHOD(GetSnapshotProperties)
		(								
		IN VSS_ID		SnapshotId, 			
		OUT VSS_SNAPSHOT_PROP	*pProp
		) = 0;												
		
	STDMETHOD(Query)
		(										
		IN VSS_ID		QueriedObjectId,		
		IN VSS_OBJECT_TYPE	eQueriedObjectType, 	
		IN VSS_OBJECT_TYPE	eReturnedObjectsType,	
		IN IVssEnumObject 	**ppEnum 				
		) = 0;												
	
	STDMETHOD(IsVolumeSupported)
		(										
		IN VSS_ID ProviderId,		
        IN VSS_PWSZ pwszVolumeName,
        IN BOOL * pbSupportedByThisProvider
		) = 0;

    STDMETHOD(DisableWriterClasses)
		(
		IN const VSS_ID *rgWriterClassId,
		IN UINT cClassId
		) = 0;

    STDMETHOD(EnableWriterClasses)
		(
		IN const VSS_ID *rgWriterClassId,
		IN UINT cClassId
		) = 0;

    STDMETHOD(DisableWriterInstances)
		(
		IN const VSS_ID *rgWriterInstanceId,
		IN UINT cInstanceId
		) = 0;

    // called to expose a snapshot 
    STDMETHOD(ExposeSnapshot)
		(
        IN VSS_ID SnapshotId,
        IN VSS_PWSZ wszPathFromRoot,
        IN LONG lAttributes,
        IN VSS_PWSZ wszExpose,
        OUT VSS_PWSZ *pwszExposed
        ) = 0;
    
	};

/*
__declspec(dllexport) HRESULT STDAPICALLTYPE CreateVssBackupComponentsXP(
    OUT IVssBackupComponentsXP **ppBackup
    );

__declspec(dllexport) HRESULT STDAPICALLTYPE CreateVssExamineWriterMetadataXP (
    IN BSTR bstrXML,
    OUT IVssExamineWriterMetadataXP **ppMetadata
    );
*/

/*
#define VSS_SW_BOOTABLE_STATE	(1 << 0)

__declspec(dllexport) HRESULT APIENTRY SimulateSnapshotFreeze (
    IN GUID         guidSnapshotSetId,
    IN ULONG        ulOptionFlags,	
    IN ULONG        ulVolumeCount,	
    IN LPWSTR      *ppwszVolumeNamesArray,
    OUT IVssAsync **ppAsync
    );

__declspec(dllexport) HRESULT APIENTRY SimulateSnapshotThaw(
    IN GUID guidSnapshotSetId
    );

__declspec(dllexport) HRESULT APIENTRY IsVolumeSnapshotted(
    IN VSS_PWSZ  pwszVolumeName,
    OUT BOOL    *pbSnapshotsPresent,
    OUT LONG	*plSnapshotCapability
    );

/////////////////////////////////////////////////////////////////////
// Life-management methods for structure members 

__declspec(dllexport) void APIENTRY VssFreeSnapshotProperties(
    IN VSS_SNAPSHOT_PROP*  pProp
    );

*/
///


#endif // _VSBACKUP_XP_H_
