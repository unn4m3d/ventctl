/*
 * Copyright (c) 2019, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MQTTClientMbedOs.h"
#include "MQTTNetworkUtil.h"

int MQTTNetworkMbedOs::read(unsigned char *buffer, int len, int timeout)
{
    return accumulate_mqtt_read(socket, buffer, len, timeout);
}

int MQTTNetworkMbedOs::write(unsigned char *buffer, int len, int timeout)
{
    return mqtt_write(socket, buffer, len, timeout);
}

int MQTTNetworkMbedOs::connect(const char *hostname, int port)
{
    SocketAddress sockAddr(hostname, port);
    return socket->connect(sockAddr);
}

int MQTTNetworkMbedOs::disconnect()
{
    return socket->close();
}

MQTTClient::MQTTClient(TCPSocket *_socket)
{
    init(_socket);
    mqttNet = new MQTTNetworkMbedOs(socket);
    client = new MQTT::Client<MQTTNetworkMbedOs, Countdown, MBED_CONF_MBED_MQTT_MAX_PACKET_SIZE, MBED_CONF_MBED_MQTT_MAX_CONNECTIONS>(*mqttNet);
};

#if defined(MBEDTLS_SSL_CLI_C) || defined(DOXYGEN_ONLY)
MQTTClient::MQTTClient(TLSSocket *_socket)
{
    init(_socket);
    mqttNet = new MQTTNetworkMbedOs(socket);
    client = new MQTT::Client<MQTTNetworkMbedOs, Countdown, MBED_CONF_MBED_MQTT_MAX_PACKET_SIZE, MBED_CONF_MBED_MQTT_MAX_CONNECTIONS>(*mqttNet);
};
#endif



MQTTClient::~MQTTClient()
{
    delete mqttNet;
    if (client != NULL) delete client;
}

nsapi_error_t MQTTClient::connect(MQTTPacket_connectData &options)
{
    if (client == NULL) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    nsapi_error_t ret = client->connect(options);
    printf("Ret: %d\n", (int)ret);
    return ret < 0 ? NSAPI_ERROR_NO_CONNECTION : ret;
}


nsapi_error_t MQTTClient::publish(const char *topicName, MQTT::Message &message)
{
    if (client == NULL) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    nsapi_error_t ret = client->publish(topicName, message);
    return ret < 0 ? NSAPI_ERROR_NO_CONNECTION : ret;
}



nsapi_error_t MQTTClient::subscribe(const char *topicFilter, enum MQTT::QoS qos, messageHandler mh)
{
    if (client == NULL) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    nsapi_error_t ret = client->subscribe(topicFilter, qos, mh);
    return ret < 0 ? NSAPI_ERROR_NO_CONNECTION : ret;
}

nsapi_error_t MQTTClient::unsubscribe(const char *topicFilter)
{
    if (client == NULL) {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    nsapi_error_t ret = client->unsubscribe(topicFilter);
    return ret < 0 ? NSAPI_ERROR_NO_CONNECTION : ret;
}

nsapi_error_t MQTTClient::yield(unsigned long timeout_ms)
{
    nsapi_error_t ret = NSAPI_ERROR_OK;
    if (client != NULL) {
        ret = client->yield(timeout_ms);
    } else {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    return ret < 0 ? NSAPI_ERROR_NO_CONNECTION : ret;
}

nsapi_error_t MQTTClient::disconnect()
{
    nsapi_error_t ret = NSAPI_ERROR_OK;
    if (client != NULL) {
        ret = client->disconnect();
    } else {
        return NSAPI_ERROR_NO_CONNECTION;
    }
    return ret < 0 ? NSAPI_ERROR_NO_CONNECTION : ret;
}

bool MQTTClient::isConnected()
{
    if ((client == NULL)){
        return false;
    } else {
        return client->isConnected();
    }
}

void MQTTClient::setDefaultMessageHandler(messageHandler mh)
{
    if (client != NULL) {
        client->setDefaultMessageHandler(mh);
    }
}

nsapi_error_t MQTTClient::setMessageHandler(const char *topicFilter, messageHandler mh)
{
    if (client == NULL) {
        return NSAPI_ERROR_NO_CONNECTION;
    } else {
        return client->setMessageHandler(topicFilter, mh);
    }
}

void MQTTClient::init(Socket *sock)
{
    socket = sock;
    client = NULL;
}
