#include "mico.h"
#include "fog_v2_include.h"

#define app_log(M, ...)             custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace()             custom_log_trace("APP")

static mico_semaphore_t wifi_sem;

#define FOG_V2_RECV_BUFF_LEN        (1024)

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

USED void user_fog_v2_ota_notification(FOG2_OTA_EVENT_TYPE ota_event_type)
{
    switch(ota_event_type)
    {
        case FOG2_OTA_CHECK_FAILURE:
            app_log("OTA EVENT: FOG2_OTA_CHECK_FAILURE");
            break;

        case FOG2_OTA_NO_NEW_VERSION:
            app_log("OTA EVENT: FOG2_OTA_NO_NEW_VERSION");
            break;

        case FOG2_OTA_IN_UPDATE:
            app_log("OTA EVENT: FOG2_OTA_IN_UPDATE");
            break;

        case FOG2_OTA_UPDATE_SUCCESS:
            app_log("OTA EVENT: FOG2_OTA_UPDATE_SUCCESS");
            break;

        case FOG2_OTA_UPDATE_FAILURE:
            app_log("OTA EVENT: FOG2_OTA_UPDATE_FAILURE");
            break;
    }

    return;
}

//接收数据线程
void fog_v2_recv( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    OSStatus err = kNoErr;
    char *fog_recv_buff = NULL;

    app_log("fog v2 recv thread start!");

    fog_recv_buff = malloc( FOG_V2_RECV_BUFF_LEN );
    require( fog_recv_buff != NULL, exit );

    while ( 1 )
    {
        memset( fog_recv_buff, 0, FOG_V2_RECV_BUFF_LEN );
        err = fog_v2_device_recv_command( fog_recv_buff, FOG_V2_RECV_BUFF_LEN, MICO_NEVER_TIMEOUT );
        if ( err == kNoErr )
        {
            app_log("recv:%s", fog_recv_buff);
        }

        app_log("recv data:%s", fog_recv_buff);
    }

    exit:
    if ( fog_recv_buff != NULL )
    {
        free( fog_recv_buff );
        fog_recv_buff = NULL;
    }

    mico_rtos_delete_thread( NULL );
}

//发送数据线程
void fog_v2_send( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    char send_buff[256] = { 0 };
    uint32_t count = 0;

    app_log("fog v2 send thread start!");

    while ( 1 )
    {
        if ( fog_v2_is_have_superuser( ) == true )
        {
            memset( send_buff, 0, sizeof(send_buff) );
            sprintf( send_buff, "{\"hello fog\" : %ld}", count ); //这种发送数据格式无法适配到mico总动员APP

            fog_v2_device_send_event( send_buff, FOG_V2_SEND_EVENT_RULES_PUBLISH | FOG_V2_SEND_EVENT_RULES_DATEBASE );
            count++;
        }

        app_log("num_of_chunks:%d,allocted_memory:%d, free:%d, total_memory:%d", MicoGetMemoryInfo()->num_of_chunks, MicoGetMemoryInfo()->allocted_memory, MicoGetMemoryInfo()->free_memory, MicoGetMemoryInfo()->total_memory);
        mico_thread_msleep( 500 );
    }

    mico_rtos_delete_thread( NULL );
}

//flash param read write test
OSStatus fog_v2_user_param_test(void)
{
    uint8_t write_buff[512] = {0};
    uint8_t read_buff[512] = {0};

    memset(write_buff, '3', sizeof(write_buff));

    fog_v2_write_user_param(write_buff, sizeof(write_buff), 0);
    fog_v2_read_user_param(read_buff, sizeof(read_buff), 0);

    if(memcmp(write_buff, read_buff, sizeof(read_buff)) == 0)
    {
        app_log("param test success!");
        return kNoErr;
    }else
    {
        app_log("param test failed!");
        return kGeneralErr;
    }
}

int application_start( void )
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

    mico_context = mico_system_context_init( sizeof(FOG_DES_S) );

    /*init fog v2 service*/
    err = init_fog_v2_service();
    require_noerr( err, exit );

    /*test user param in flash*/
    err = fog_v2_user_param_test();
    require_noerr( err, exit );

    err = mico_system_init( mico_context );
    require_noerr( err, exit );

    /* wait for wifi on */
    mico_rtos_get_semaphore( &wifi_sem, MICO_WAIT_FOREVER );

    err = start_fog_v2_service( );
    require_noerr( err, exit );

    /* Create send thread */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "fog_v2_send_thread", fog_v2_send, 0x800, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the fog_v2_send_thread." );

    /* Create recv thread */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "fog_v2_recv_thread", fog_v2_recv, 0x800, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the fog_v2_recv_thread." );

    exit:
    if(err != kNoErr)
    {
        MicoSystemReboot( );
    }

    mico_rtos_delete_thread( NULL );
    return err;
}
