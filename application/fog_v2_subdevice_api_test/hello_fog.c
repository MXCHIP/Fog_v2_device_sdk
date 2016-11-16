#include "mico.h"
#include "fog_v2_include.h"
#include "fog_v2_subdevice_app.h"

#define app_log(M, ...)             custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace()             custom_log_trace("APP")

static mico_semaphore_t wifi_sem;

#define FOG_V2_RECV_BUFF_LEN        (2048)

void appNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            app_log("Wi-Fi connected.");
            mico_rtos_set_semaphore( &wifi_sem );
            break;
        case NOTIFY_STATION_DOWN:
            app_log("Wi-Fi disconnected.");
            break;
        default:
            break;
    }
}

//echoÏß³Ì
void fog_v2_echo( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    OSStatus err = kNoErr;
    char *fog_recv_buff = NULL;

    app_log("fog v2 send thread start!");

    fog_recv_buff = malloc( FOG_V2_RECV_BUFF_LEN );
    require( fog_recv_buff != NULL, exit );

    while(1)
    {
        memset(fog_recv_buff, 0, FOG_V2_RECV_BUFF_LEN);
        err = fog_v2_device_recv_command( fog_recv_buff, FOG_V2_RECV_BUFF_LEN, MICO_NEVER_TIMEOUT );
        if ( err == kNoErr )
        {
            app_log("recv:%s", fog_recv_buff);
        }else
        {
            app_log("fog_v2_device_recv_command() error");
        }

        fog_v2_device_send_event(fog_recv_buff, FOG_V2_SEND_EVENT_RULES_PUBLISH);
    }

exit:
    if(fog_recv_buff != NULL)
    {
        free(fog_recv_buff);
        fog_recv_buff = NULL;
    }

    mico_rtos_delete_thread( NULL );
}

int application_start(void)
{
	app_log_trace();
	OSStatus err = kNoErr;
    mico_Context_t* mico_context;

    app_log("APP Version:%s%s", FOG_V2_REPORT_VER, FOG_V2_REPORT_VER_NUM);

    err = mico_rtos_init_semaphore( &wifi_sem, 1 );
    require_noerr( err, exit );

    /* Register user function for MiCO notification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *) appNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit );

	mico_context = mico_system_context_init(sizeof(FOG_DES_S));

    /*init fog v2 service*/
    err = init_fog_v2_service();
    require_noerr( err, exit );

	err = mico_system_init(mico_context);
	require_noerr( err, exit);

    /* wait for wifi on */
    mico_rtos_get_semaphore( &wifi_sem, MICO_WAIT_FOREVER );

    app_log("[111]num_of_chunks:%d,allocted_memory:%d, free:%d, total_memory:%d", MicoGetMemoryInfo()->num_of_chunks, MicoGetMemoryInfo()->allocted_memory, MicoGetMemoryInfo()->free_memory, MicoGetMemoryInfo()->total_memory);

    err = start_fog_v2_service();
    require_noerr( err, exit);

    /*subdevice api test*/
    fog_sub_api_test();

    /* Create send thread */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "fog_v2_send_thread", fog_v2_echo, 0x800, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the fog_v2_send_thread." );

	exit:
	mico_rtos_delete_thread(NULL);
	return err;
}
