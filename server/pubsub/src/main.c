#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "publish.h"
#include "subscribe.h"
#include "printer.h"
#include "free.h"
#include "parser.h"
#include "dotenv.h"
#include "server.h"

static char* getConfigPath(){
    const char *dir = getenv("CONFIG_DOCKER_DIR");
    const char *name = getenv("CONFIG_NAME");

    if (!dir || !name) {
        fprintf(stderr, "Missing CONFIG_DOCKER_DIR or CONFIG_NAME environment variable.\n");
        return "";
    }

    size_t len = strlen(dir) + 1 + strlen(name) + 1;
    char *configPath = malloc(len);
    if (!configPath) {
        perror("malloc");
        return "";
    }

    snprintf(configPath, len, "%s/%s", dir, name);

    return configPath;
}

bool getenv_bool(const char *name, bool default_value) {
    const char *val = getenv(name);
    if (!val) return default_value;

    if (strcasecmp(val, "1") == 0 ||
        strcasecmp(val, "true") == 0 ||
        strcasecmp(val, "yes") == 0 ||
        strcasecmp(val, "on") == 0) 
        return true;

    if (strcasecmp(val, "0") == 0 ||
        strcasecmp(val, "false") == 0 ||
        strcasecmp(val, "no") == 0 ||
        strcasecmp(val, "off") == 0)
        return false;

    return default_value;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    if (env_load("/app/.env", true) != 0) {
        perror("env_load");
    }

    char *role = getenv("ROLE");
    int number = atoi(getenv("ID"));  
    char *configPath = getConfigPath();
    bool debug = getenv_bool("DEBUG", false);
    char *brokerIP = getenv("BROKER_IP");
    if(!role) {
        fprintf(stderr, "Error: missing -role pub|sub\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("DEBUG=%b\n", debug);
    printf("Using config file: %s\n", configPath);

    SystemConfig *sysConf = parseFile(configPath, -1);

    char* subscribeProtocol = getenv("SUBSCRIBE_PROTOCOL");
    char* publishProtocol = getenv("PUBLISH_PROTOCOL");

    printf("%s\n", subscribeProtocol);
    UA_String subscribeTransportProfile = UA_STRING_NULL;
    UA_String publishTransportProfile = UA_STRING_NULL;

    UA_NetworkAddressUrlDataType subscribeNetworkAddressUrl = {UA_STRING_NULL, UA_STRING_NULL};
    UA_NetworkAddressUrlDataType publishNetworkAddressUrl = {UA_STRING_NULL, UA_STRING_NULL};

    if (strcmp(subscribeProtocol, "UADP") == 0){
        subscribeTransportProfile =
            UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
        subscribeNetworkAddressUrl =
            (UA_NetworkAddressUrlDataType){UA_STRING_NULL, UA_STRING("opc.udp://224.168.10.102:4840/")};
    } else{
        fprintf(stderr, "Protocol missing\n");
        return EXIT_FAILURE;
    }

    if (strcmp(publishProtocol, "MQTT") == 0) {
        publishTransportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json");
        publishNetworkAddressUrl = (UA_NetworkAddressUrlDataType){
            UA_STRING_NULL,
            UA_STRING(brokerIP)
        };
    } else if (strcmp(publishProtocol, "UADP") == 0){
        publishTransportProfile =
            UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
        publishNetworkAddressUrl =
            (UA_NetworkAddressUrlDataType){UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4841/")};
    }
    else {
        fprintf(stderr, "Publish protocol missing\n");
        return EXIT_FAILURE;
    }
    
    if(strcmp(role, "pub") == 0) {
        return runPublish(&subscribeTransportProfile, &subscribeNetworkAddressUrl, sysConf, number, debug);
    } else if(strcmp(role, "bridge") == 0) {
        return runServer(&subscribeTransportProfile, &subscribeNetworkAddressUrl, &publishTransportProfile, &publishNetworkAddressUrl, sysConf, debug);
    } else if (strcmp(role, "sub") == 0) {
        return runSubscribe(&publishTransportProfile, &publishNetworkAddressUrl, sysConf, debug);
    }
    else {
        fprintf(stderr, "Error: unknown role '%s'\n", role);
        usage(argv[0]);
        return EXIT_FAILURE;
    }
}
