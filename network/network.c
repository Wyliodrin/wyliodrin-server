/**************************************************************************************************
 * Networking implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: December 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

#include "../winternals/winternals.h" /* logs and errs */

#include "network.h" /* API */

/*************************************************************************************************/



/*** API IMPLEMENTATION **************************************************************************/

network_list_t *get_hosts() {
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];
  network_list_t *result = NULL;

  wsyserr2(getifaddrs(&ifaddr) == -1, return NULL, "getifaddrs error");

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL) {
      continue;
    }

    family = ifa->ifa_addr->sa_family;

    if (family != AF_INET || strncmp(ifa->ifa_name, "lo", 2) == 0) {
      continue;
    }

    s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                          sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
    werr2(s != 0, continue, "getnameinfo() failed: %s", gai_strerror(s));

    printf("%s <%s>\n", ifa->ifa_name, host);
    network_list_t *elem = malloc(sizeof(network_list_t));
    wsyserr2(elem == NULL, continue, "Memory allocation failed");
    elem->name = strdup(ifa->ifa_name);
    elem->host = strdup(host);
    elem->next = NULL;
    if (result == NULL) {
      result = elem;
    } else {
      result->next = elem;
    }
  }

  freeifaddrs(ifaddr);

  return result;
}

/*************************************************************************************************/
