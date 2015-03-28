/*
 * log_topn.h
 * Copyright (C) 2015 liuben <liuben@ubuntu>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LOG_TOPN_H
#define LOG_TOPN_H

#define CIP_P 3
#define VIEW_P 6
#define DOMAIN_P 7
#define RTYPE_P 9
#define RCODE_P 10
#define ZONE_P 17

typedef struct house_keeper house_keeper_t;
void house_keeper_destroy(house_keeper_t *keeper);
house_keeper_t *house_keeper_create();
void log_handle(house_keeper_t *keeper, const char *view, const char *domain,
        const char *ip, const char *rtype, const char *rcode);

#endif /* !LOG_TOPN_H */
