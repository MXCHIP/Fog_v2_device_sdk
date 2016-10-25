#include "mico.h"
#include "fog_v2_config.h"
#include "fog_v2_subdevice_app.h"

#define app_log(M, ...)             custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace()             custom_log_trace("APP")

void fog_sub_api_test(void);

//设备状态改变通知
USED void user_fog_v2_device_notification(SUBDEVICE_CMD_TYPE type, const char *s_product_id, const char *s_mac)
{
    if(type == MQTT_CMD_SUB_UNBIND)
    {
        if(s_product_id == NULL || s_mac == NULL)
        {
            app_log("param error!");
            return;
        }

        app_log("subdevice is unbind, product id:%s, mac: %s", s_product_id, s_mac);
    }else if(type == MQTT_CMD_GATEWAY_UNBIND)
    {
        app_log("gateway is unbind!!! subdevice can't send data");
    }else if(type == MQTT_CMD_GATEWAY_BIND)
    {
        app_log("gateway is bind!!! subdevice can send data");
    }

    return;
}


//子设备API测试
void sub_device_test(mico_thread_arg_t arg)
{
    OSStatus err = kGeneralErr;
    char recv_buff[100] = {0};

    while(fog_v2_is_have_superuser() == false)
    {
        mico_thread_msleep(200);
    }

    app_log("\r\n##########################\r\n");

//    //添加子设备
//    err = fog_v2_add_subdevice(FOG_V2_SUB_PRODUCT_ID_TEMP, FOG_V2_SUB_MAC_TEMP);
//    require_noerr( err, exit );

    //删除子设备
    err = fog_v2_remove_subdevice(FOG_V2_SUB_PRODUCT_ID_TEMP, FOG_V2_SUB_MAC_TEMP);
    if(err != kNoErr)
    {
        app_log("remove failed");
    }

    //添加子设备
    err = fog_v2_add_subdevice(FOG_V2_SUB_PRODUCT_ID_TEMP, FOG_V2_SUB_MAC_TEMP, true);
    require_noerr( err, exit );


    while(1)
    {
        err = fog_v2_subdevice_send( FOG_V2_SUB_PRODUCT_ID_TEMP, FOG_V2_SUB_MAC_TEMP,  "{\"111\":123}", FOG_V2_SEND_EVENT_RULES_PUBLISH | FOG_V2_SEND_EVENT_RULES_DATEBASE);
        require_noerr( err, exit );

        mico_thread_sleep(2);

        memset(recv_buff, 0, sizeof(recv_buff));
        err = fog_v2_subdevice_recv(FOG_V2_SUB_PRODUCT_ID_TEMP, FOG_V2_SUB_MAC_TEMP, recv_buff, sizeof(recv_buff), MICO_WAIT_FOREVER);
        if(err != kNoErr)
        {
            app_log("recv error!");
        }

        app_log("recv:%s", recv_buff);
    }

 exit:
    mico_rtos_delete_thread(NULL);
}


void fog_sub_api_test(void)
{
    OSStatus err = kGeneralErr;

    /* Create a new thread */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "sub_device_test", sub_device_test, 0x1000, (uint32_t)NULL );
    require_noerr_string( err, exit, "ERROR: Unable to start the sub_device_test" );

 exit:
    return;
}
