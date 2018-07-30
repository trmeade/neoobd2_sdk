#ifndef _MQTT_DEMO_H_
#define _MQTT_DEMO_H_

#define demoDECLARE_DEMO( f )    extern void f( void )

void DEMO_RUNNER_RunDemos( void );

demoDECLARE_DEMO( vStartMQTTEchoDemo );

#endif
