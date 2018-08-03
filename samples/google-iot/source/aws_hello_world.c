/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "MQTTClient.h"
#include "message_buffer.h"

/* MQTT includes. */
#include "aws_mqtt_agent.h"

/* Credentials includes. */
#include "aws_clientcredential.h"

/* Demo includes. */
#include "aws_demo_config.h"
#include "aws_hello_world.h"

// ISM Lib includes
#include "obd2pro_wifi_cc32xx_ism.h"
#include "obd2pro_wifi_cc32xx.h"



/**
 * @brief MQTT client ID.
 *
 * It must be unique per MQTT broker.
 */
#define echoCLIENT_ID          ((const uint8_t *)"MQTTEcho")

/**
 * @brief The topic that the MQTT client both subscribes and publishes to.
 */
#define echoTOPIC_NAME         ((const uint8_t *)"freertos/demos/echo")

/**
 * @brief The string appended to messages that are echoed back to the MQTT broker.
 *
 * It is also used to detect if a received message has already been acknowledged.
 */
#define echoACK_STRING         ((const char *)" ACK")

/**
 * @brief Dimension of the character array buffers used to hold data (strings in
 * this case) that is published to and received from the MQTT broker (in the cloud).
 */
#define echoMAX_DATA_LENGTH    20

/**
 * @brief A block time of 0 simply means "don't block".
 */
#define echoDONT_BLOCK         ((TickType_t)0)

/**
 * @brief Network ID of CAN Channel.
 */
#define echoHS_CAN1_NW_ID      1

/**
 * @brief Network ID of CAN Channel.
 */
#define echoCAN_DATA_LEN       8

/*-----------------------------------------------------------*/

/**
 * @brief Implements the task that connects to and then publishes messages to the
 * MQTT broker.
 *
 * Messages are published every five seconds for a minute.
 *
 * @param[in] pvParameters Parameters passed while creating the task. Unused in our
 * case.
 */
static void prvMQTTEchoTask(void *pvParameters);

void messageArrived(MessageData* data);

/*-----------------------------------------------------------*/
static void prvTxGenericCAN (char * cCANData, size_t xCANLen, int iArbID )
{
    if (cCANData != NULL)
    {
        size_t xlen;
        GenericMessage message = { 0 };

        message.iNetwork = echoHS_CAN1_NW_ID;
        message.iID = iArbID;
        xlen = (xCANLen > echoCAN_DATA_LEN)?echoCAN_DATA_LEN:xCANLen;
        memcpy(message.btData, cCANData, xlen);
        message.iNumDataBytes = xlen;

        GenericMessageTransmit(&message);
    }
    else {
        configPRINTF(("CAN transmit failed due to bad pointer\r\n"));
    }
}


void messageArrived(MessageData* data)
{
    char cCANMsg[8] = {0x01,0,0,0,0,0,0,0x01};

    configPRINTF(("Message arrived on topic %.*s: %.*s\r\n\r\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
        data->message->payloadlen, data->message->payload));
         prvTxGenericCAN(cCANMsg, echoCAN_DATA_LEN, 0x01);
}

/*-----------------------------------------------------------*/


static void prvISMTask(void * arg)
{
    (void) arg;

    while (1)
    {
        /* This loop must be called without any delay to ensure
         * fast communication between CC3220SF and the main
         * data acquisition processor */
        icsISM_Main();
    }
}
/*-----------------------------------------------------------*/

static void prvMQTTEchoTask(void *pvParameters)
{

    /* Avoid compiler warnings about unused parameters. */
    (void) pvParameters;

    /* Create the MQTT client object and connect it to the MQTT broker. */
    //xReturned = prvCreateClientAndConnectToBroker();

    /* connect to m2m.eclipse.org, subscribe to a topic, send and receive messages regularly every 1 sec */
       MQTTClient client;
       Network network;
       unsigned char sendbuf[80], readbuf[80];
       int rc = 0,
           count = 0;
       MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

       pvParameters = 0;
       NetworkInit(&network);
       MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

       char* address = "test.mosquitto.org";
       if ((rc = NetworkConnect(&network, address, 1883)) != 0)
       {
         configPRINTF(("Return code from network connect is %d\r\n", rc));
         goto exit;
       }


   #if defined(MQTT_TASK)
       if ((rc = MQTTStartTask(&client)) != pdPASS)
           configPRINTF(("Return code from start tasks is %d\r\n", rc));
   #endif

       connectData.MQTTVersion = 3;
       connectData.clientID.cstring = "FreeRTOS_sample";

       if ((rc = MQTTConnect(&client, &connectData)) == 0)
       {
           configPRINTF(("MQTT Connected\r\n\r\n"));
           if ((rc = MQTTSubscribe(&client, "ICS/sample/#", QOS0, messageArrived)) == 0)
           {
               do
               {
                    MQTTMessage message;
                    char payload[30];

                    message.qos = QOS0;
                    message.retained = 0;
                    message.payload = payload;
                    sprintf(payload, "message number %d", count);
                    message.payloadlen = strlen(payload);

                    if ((rc = MQTTPublish(&client, "ICS/sample/a", &message)) == 0)
                    {
                      #if !defined(MQTT_TASK)
                          if ((rc = MQTTYield(&client, 1000)) != 0)
                              configPRINTF(("Return code from yield is %d\r\n", rc));
                      #endif
                    }
                    else {
                      configPRINTF(("Return code from MQTT publish is %d\r\n", rc));
                    }

                    ++count;
                } while (count < 5);
           }
           else {
               configPRINTF(("Return code from MQTT subscribe is %d\r\n", rc));
           }
       }
       else
       {
            configPRINTF(("Return code from MQTT connect is %d\r\n", rc));
       }

       /* do not return */

exit:
    configPRINTF( ( "MQTT echo demo finished.\r\n" ) );
    vTaskDelete( NULL ); /* Delete this task. */
}
/*-----------------------------------------------------------*/

void vStartMQTTEchoDemo(void)
{
    configPRINTF( ( "Creating MQTT Echo Task...\r\n" ) );

    /* Create the message buffer used to pass strings from the MQTT callback
     * function to the task that echoes the strings back to the broker.  The
     * message buffer will only ever have to hold one message as messages are only
     * published every 5 seconds.  The message buffer requires that there is space
     * for the message length, which is held in a size_t variable. */


    (void) xTaskCreate(prvMQTTEchoTask, "MQTTEcho0",
                       democonfigMQTT_ECHO_TASK_STACK_SIZE, NULL,
                       democonfigMQTT_ECHO_TASK_PRIORITY, NULL);


    (void) xTaskCreate(prvISMTask, /* The function that implements the demo task. */
                       "ISM", /* The name to assign to the task being created. */
                       democonfigISM_TASK_STACK_SIZE, /* The size, in WORDS (not bytes), of the stack to allocate for the task being created. */
                       NULL, /* The task parameter is not being used. */
                       democonfigISM_TASK_PRIORITY, /* The priority at which the task being created will run. */
                       NULL); /* Not storing the task's handle. */
}
/*-----------------------------------------------------------*/
