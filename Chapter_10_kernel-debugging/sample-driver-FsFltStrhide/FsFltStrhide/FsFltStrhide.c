/*++

Module Name:

    FsFltStrhide.c

Abstract:

    This is the main module of the FsFltStrhide miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
#define PTDBG_TRACE_ERROR               0x00000004

ULONG gTraceFlags = 0x7;

#if FALSE // Compile-time selection of particularly verbose debug outputs
    #define ENABLE_VERBOSE_FILE_NAME_INFO
#endif


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

#define PT_DBG_PRINTEX( _ctDbgLevel, _dpfltDbgLevel, _fstring, ... ) \
    (FlagOn(gTraceFlags,(_ctDbgLevel)) ? \
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, (_dpfltDbgLevel), (_fstring), __VA_ARGS__ ) : \
        ((int)0))


/*************************************************************************
Pool Tags
*************************************************************************/

#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'

/*************************************************************************
Local structures
*************************************************************************/

//
//  This is a volume context, one of these are attached to each volume
//  we monitor.  This is used to get a "DOS" name for debug display.
//

typedef struct _VOLUME_CONTEXT {

    //
    //  Holds the name to display
    //

    UNICODE_STRING Name;

    //
    //  Holds the sector size for this volume.
    //

    ULONG SectorSize;

} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

#define MIN_SECTOR_SIZE 0x200

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
FsFltStrhideInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
CleanupVolumeContext(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
FsFltStrhideInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
FsFltStrhideInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
FsFltStrhideUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
FsFltStrhideInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
StrhidePreRead(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FsFltStrhideUnload)
#pragma alloc_text(PAGE, FsFltStrhideInstanceQueryTeardown)
#pragma alloc_text(PAGE, FsFltStrhideInstanceSetup)
#pragma alloc_text(PAGE, CleanupVolumeContext)
#pragma alloc_text(PAGE, FsFltStrhideInstanceTeardownStart)
#pragma alloc_text(PAGE, FsFltStrhideInstanceTeardownComplete)
#pragma alloc_text(PAGE, StrhidePreRead)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

#if 0 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_CLOSE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },
#endif
    { IRP_MJ_READ,
      0,
      StrhidePreRead,
      NULL }, //optional
#if 0
    { IRP_MJ_WRITE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_SET_EA,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      FsFltStrhidePreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_CLEANUP,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_PNP,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_MDL_READ,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      FsFltStrhidePreOperation,
      FsFltStrhidePostOperation },

#endif // TODO

    { IRP_MJ_OPERATION_END }
};

//
//  Context definitions we currently care about.  Note that the system will
//  create a lookAside list for the volume context because an explicit size
//  of the context is specified.
//

CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

     { FLT_VOLUME_CONTEXT,
       0,
       CleanupVolumeContext,
       sizeof(VOLUME_CONTEXT),
       CONTEXT_TAG },

     { FLT_CONTEXT_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextNotifications,               //  Context
    Callbacks,                          //  Operation callbacks

    FsFltStrhideUnload,                           //  MiniFilterUnload

    FsFltStrhideInstanceSetup,                    //  InstanceSetup
    FsFltStrhideInstanceQueryTeardown,            //  InstanceQueryTeardown
    FsFltStrhideInstanceTeardownStart,            //  InstanceTeardownStart
    FsFltStrhideInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
FsFltStrhideInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume.

    By default we want to attach to all volumes.  This routine will try and
    get a "DOS" name for the given volume.  If it can't, it will try and
    get the "NT" name for the volume (which is what happens on network
    volumes).  If a name is retrieved a volume context will be created with
    that name.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    PDEVICE_OBJECT devObj = NULL;
    PVOLUME_CONTEXT ctx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG retLen;
    PUNICODE_STRING workingName;
    USHORT size;
    UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512];
    PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFltStrhide!FsFltStrhideInstanceSetup: Entered\n") );

    try {

        //
        //  Allocate a volume context structure.
        //

        status = FltAllocateContext( FltObjects->Filter,
                                     FLT_VOLUME_CONTEXT,
                                     sizeof(VOLUME_CONTEXT),
                                     NonPagedPool,
                                     &ctx );

        if (!NT_SUCCESS(status)) {

            //
            //  We could not allocate a context, quit now
            //

            PT_DBG_PRINTEX( PTDBG_TRACE_ERROR, DPFLTR_ERROR_LEVEL,
                "FsFltStrhide!FsFltStrhideInstanceSetup: Failed to create volume context with status 0x%x. (FltObjects @ %p)\n",
                status,
                FltObjects );

            leave;
        }

        //
        //  Always get the volume properties, so I can get a sector size
        //

        status = FltGetVolumeProperties( FltObjects->Volume,
                                         volProp,
                                         sizeof(volPropBuffer),
                                         &retLen );

        if (!NT_SUCCESS(status)) {

            leave;
        }

        //
        //  Save the sector size in the context for later use.  Note that
        //  we will pick a minimum sector size if a sector size is not
        //  specified.
        //

        FLT_ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= MIN_SECTOR_SIZE));

        ctx->SectorSize = max(volProp->SectorSize,MIN_SECTOR_SIZE);

        //
        //  Init the buffer field (which may be allocated later).
        //

        ctx->Name.Buffer = NULL;

        //
        //  Get the storage device object we want a name for.
        //

        status = FltGetDiskDeviceObject( FltObjects->Volume, &devObj );

        if (NT_SUCCESS(status)) {

            //
            //  Try and get the DOS name.  If it succeeds we will have
            //  an allocated name buffer.  If not, it will be NULL
            //

            status = IoVolumeDeviceToDosName( devObj, &ctx->Name );
        }

        //
        //  If we could not get a DOS name, get the NT name.
        //

        if (!NT_SUCCESS(status)) {

            FLT_ASSERT(ctx->Name.Buffer == NULL);

            //
            //  Figure out which name to use from the properties
            //

            if (volProp->RealDeviceName.Length > 0) {

                workingName = &volProp->RealDeviceName;

            } else if (volProp->FileSystemDeviceName.Length > 0) {

                workingName = &volProp->FileSystemDeviceName;

            } else {

                //
                //  No name, don't save the context
                //

                status = STATUS_FLT_DO_NOT_ATTACH;
                leave;
            }

            //
            //  Get size of buffer to allocate.  This is the length of the
            //  string plus room for a trailing colon.
            //

            size = workingName->Length + sizeof(WCHAR);

            //
            //  Now allocate a buffer to hold this name
            //

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "ctx->Name.Buffer will not be leaked because it is freed in CleanupVolumeContext")
            ctx->Name.Buffer = ExAllocatePoolWithTag( NonPagedPool,
                                                      size,
                                                      NAME_TAG );
            if (ctx->Name.Buffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

            //
            //  Init the rest of the fields
            //

            ctx->Name.Length = 0;
            ctx->Name.MaximumLength = size;

            //
            //  Copy the name in
            //

            RtlCopyUnicodeString( &ctx->Name,
                                  workingName );

            //
            //  Put a trailing colon to make the display look good
            //

            RtlAppendUnicodeToString( &ctx->Name,
                                      L":" );
        }

        //
        //  Set the context
        //

        status = FltSetVolumeContext( FltObjects->Volume,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      ctx,
                                      NULL );

        //
        //  Log debug info
        //

        PT_DBG_PRINTEX( PTDBG_TRACE_OPERATION_STATUS, DPFLTR_TRACE_LEVEL,
            "FsFltStrhide!FsFltStrhideInstanceSetup:                  Real SectSize=0x%04x, Used SectSize=0x%04x, Name=\"%wZ\"\n",
                    volProp->SectorSize,
                    ctx->SectorSize,
                    &ctx->Name);

        //
        //  It is OK for the context to already be defined.
        //

        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

            status = STATUS_SUCCESS;
        }

    } finally {

        //
        //  Always release the context.  If the set failed, it will free the
        //  context.  If not, it will remove the reference added by the set.
        //  Note that the name buffer in the ctx will get freed by the context
        //  cleanup routine.
        //

        if (ctx) {

            FltReleaseContext( ctx );
        }

        //
        //  Remove the reference added to the device object by
        //  FltGetDiskDeviceObject.
        //

        if (devObj) {

            ObDereferenceObject( devObj );
        }
    }

    return status;
}


VOID
CleanupVolumeContext(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    The given context is being freed.
    Free the allocated name buffer if there one.

Arguments:

    Context - The context being freed

    ContextType - The type of context this is

Return Value:

    None

--*/
{
    PVOLUME_CONTEXT ctx = Context;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( ContextType );

    FLT_ASSERT(ContextType == FLT_VOLUME_CONTEXT);

    if (ctx->Name.Buffer != NULL) {

        ExFreePool(ctx->Name.Buffer);
        ctx->Name.Buffer = NULL;
    }
}


NTSTATUS
FsFltStrhideInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFltStrhide!FsFltStrhideInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
FsFltStrhideInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFltStrhide!FsFltStrhideInstanceTeardownStart: Entered\n") );
}


VOID
FsFltStrhideInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFltStrhide!FsFltStrhideInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFltStrhide!DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}

NTSTATUS
FsFltStrhideUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFltStrhide!FsFltStrhideUnload: Entered\n") );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
StrhidePreRead(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++
Routine Description:

    This routine detects the presence of the target byte sequence
    and prints debug output on where it has been detected.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOperation callback

    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOLUME_CONTEXT volCtx = NULL;
    PFLT_FILE_NAME_INFORMATION filenameInfo = NULL;

    NTSTATUS status;
    ULONG readLen = iopb->Parameters.Read.Length;

    UNREFERENCED_PARAMETER(CompletionContext);

    try {

        //
        //  If they are trying to read ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        //

        if (readLen == 0) {

            leave;
        }

        //
        //  Get file name
        //

        status = FltGetFileNameInformation( Data,
                                            FLT_FILE_NAME_NORMALIZED |
                                            FLT_FILE_NAME_QUERY_DEFAULT,
                                            &filenameInfo );

        if (NT_SUCCESS( status )) {

            status = FltParseFileNameInformation( filenameInfo );

            if (NT_SUCCESS( status )) {

#ifdef ENABLE_VERBOSE_FILE_NAME_INFO
                const static UNICODE_STRING nullstr = RTL_CONSTANT_STRING( L"(null)" );

                PT_DBG_PRINTEX( PTDBG_TRACE_OPERATION_STATUS, DPFLTR_INFO_LEVEL,
                    "FsFltStrhide!StrhidePreRead: Obtained file name information:\n"
                        "\t Name      : %wZ\n"
                        "\t Volume    : %wZ\n"
                        "\t Share     : %wZ\n"
                        "\t Extension : %wZ\n"
                        "\t Stream    : %wZ\n"
                        "\t Fin.Comp  : %wZ\n"
                        "\t ParentDir : %wZ\n",
                    &filenameInfo->Name,
                    &filenameInfo->Volume ? &filenameInfo->Volume : &nullstr,
                    &filenameInfo->Share ? &filenameInfo->Share : &nullstr,
                    &filenameInfo->Extension ? &filenameInfo->Extension : &nullstr,
                    &filenameInfo->Stream ? &filenameInfo->Stream : &nullstr,
                    &filenameInfo->FinalComponent ? &filenameInfo->FinalComponent : &nullstr,
                    &filenameInfo->ParentDir ? &filenameInfo->ParentDir : &nullstr );
#endif
            }
            else { // Failed to parse file information

                PT_DBG_PRINTEX( PTDBG_TRACE_OPERATION_STATUS, DPFLTR_INFO_LEVEL | DPFLTR_WARNING_LEVEL,
                    "FsFltStrhide!StrhidePreRead: Obtained file name information (parse failed):\n"
                        "\t Name      : %wZ\n",
                    &filenameInfo->Name );
            }
        }
        else {

            PT_DBG_PRINTEX( PTDBG_TRACE_ERROR, DPFLTR_ERROR_LEVEL,
                "FsFltStrhide!StrhidePreRead: Failed to get file name information (CallbackData = %p, FileObject = %p)\n",
                Data,
                FltObjects->FileObject );

            leave;
        }


        //
        //  Get our volume context so we can display our volume name in the
        //  debug output.
        //

        status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volCtx );

        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINTEX( PTDBG_TRACE_ERROR, DPFLTR_ERROR_LEVEL,
                "FsFltStrhide!StrhidePreRead: Error getting volume context, status=%x\n",
                status);

            leave;
        }

        PT_DBG_PRINTEX( PTDBG_TRACE_OPERATION_STATUS, DPFLTR_TRACE_LEVEL,
            "FsFltStrhide!StrhidePreRead: %wZ bufaddr=%p mdladdr=%p len=%d\n",
            &volCtx->Name,
            iopb->Parameters.Read.ReadBuffer,
            iopb->Parameters.Read.MdlAddress,
            readLen);

        //
        //  Determine whether file content is to be concealed by extension
        //

        const static UNICODE_STRING targetext = RTL_CONSTANT_STRING( L"!hid" );
        if ( (&filenameInfo->Extension != NULL) && (RtlEqualUnicodeString(&filenameInfo->Extension, &targetext, TRUE)) ) {

            PT_DBG_PRINTEX( PTDBG_TRACE_OPERATION_STATUS, DPFLTR_INFO_LEVEL,
                "FsFltStrhide!StrhidePreRead: Detected file marked for concealment: %wZ\n",
                &filenameInfo->Name );

            //
            // Invalidate read buffer
            //

            iopb->Parameters.Read.Length = 0;
            iopb->Parameters.Read.ReadBuffer = NULL;
            iopb->Parameters.Read.MdlAddress = NULL;
            FltSetCallbackDataDirty( Data );
        }

    } finally {

        //
        //  Cleanup state.
        //

        if (volCtx != NULL) {
            FltReleaseContext( volCtx );
        }

        if (filenameInfo != NULL) {
            FltReleaseFileNameInformation( filenameInfo );
        }
    }

    return retValue;
}
