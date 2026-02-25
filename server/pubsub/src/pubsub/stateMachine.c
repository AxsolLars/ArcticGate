// 
// #include <open62541/plugin/log_stdout.h>
// #include <open62541/server.h>
// #include <open62541/server_pubsub.h>
// #include <open62541/util.h>
// static UA_StatusCode
// writerGroupStateMachine(UA_Server *server, const UA_NodeId componentId,
//                         void *componentContext, UA_PubSubState *state,
//                         UA_PubSubState targetState) {
//     UA_WriterGroupConfig config;
//     struct itimerspec interval;
//     memset(&interval, 0, sizeof(interval));
// 
//     if(targetState == *state)
//         return UA_STATUSCODE_GOOD;
//     
//     switch(targetState) {
//         /* Disabled or Error */
//         case UA_PUBSUBSTATE_ERROR:
//         case UA_PUBSUBSTATE_DISABLED:
//         case UA_PUBSUBSTATE_PAUSED:
//             printf("XXX Disabling the WriterGroup\n");
//             timer_settime(writerGroupTimer, 0, &interval, NULL);
//             *state = targetState;
//             break;
// 
//         /* Operational */
//         case UA_PUBSUBSTATE_PREOPERATIONAL:
//         case UA_PUBSUBSTATE_OPERATIONAL:
//             if(*state == UA_PUBSUBSTATE_OPERATIONAL)
//                 break;
//             printf("XXX Enabling the WriterGroup\n");
//             UA_Server_getWriterGroupConfig(server, writerGroupIdent, &config);
//             interval.it_interval.tv_sec = config.publishingInterval / 1000;
//             interval.it_interval.tv_nsec =
//                 ((long long)(config.publishingInterval * 1000 * 1000)) % (1000 * 1000 * 1000);
//             interval.it_value = interval.it_interval;
//             UA_WriterGroupConfig_clear(&config);
//             int res = timer_settime(writerGroupTimer, 0, &interval, NULL);
//             if(res != 0)
//                 return UA_STATUSCODE_BADINTERNALERROR;
//             *state = UA_PUBSUBSTATE_OPERATIONAL;
//             break;
// 
//         /* Unknown state */
//         default:
//             return UA_STATUSCODE_BADINTERNALERROR;
//     }
// 
//     return UA_STATUSCODE_GOOD;
// }
// 
// static UA_StatusCode
// State_offsetComponentLifecycleCallback(UA_Server *server, const UA_NodeId id,
//                                const UA_PubSubComponentType componentType,
//                                UA_Boolean remove) {
//     if(remove)
//         return UA_STATUSCODE_GOOD;
//     if(componentType != UA_PUBSUBCOMPONENT_WRITERGROUP)
//         return UA_STATUSCODE_GOOD;
// 
//     printf("XXX Set the custom state machine for the new WriterGroup\n");
// 
//     UA_WriterGroupConfig c;
//     UA_Server_getWriterGroupConfig(server, id, &c);
//     c.customStateMachine = writerGroupStateMachine;
//     UA_Server_updateWriterGroupConfig(server, id, &c);
//     UA_WriterGroupConfig_clear(&c);
// 
//     return UA_STATUSCODE_GOOD;
// }
