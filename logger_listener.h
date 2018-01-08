#ifndef LOGGER_LISTENER_H_H
#define LOGGER_LISTENER_H_H

#include "event_listener.h"

EventListener* logger_listener_create(int logger_level, FILE *fp);

#endif //LOGGER_LISTENER_H_H
