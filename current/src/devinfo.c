/* IPwatchD - IP conflict detection in Linux systems
 * Copyright (C) 2007 Jaroslav Imrich <jariq@jariq.sk>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */


#include "ipwatchd.h"


extern int   debug_flag;               /* Flag indicating debug mode */
extern char  msgbuf[IPWD_MSG_BUFSIZ];  /* Buffer for output messages */


/* ipwd_devinfo - gets the IP and MAC addresses of specified device in human readable form
 * 
 * Parameters:
 *   - p_dev - name of the device (i.e. eth0)
 *   - p_ip  - pointer to string where the IP should be stored
 *   - p_mac - pointer to string where the MAC should be stored
 * 
 * Based on examples from:
 * http://english.geekpage.jp/programming/linux-network/
 */

int ipwd_devinfo(char *p_dev, char *p_ip, char *p_mac) {

  /* Create UDP socket */
  int sock = -1;
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Could not open socket.\n");
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }

  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strcpy(ifr.ifr_name, p_dev);

  /* Get IP address of interface */
  if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Could not retrieve IP address of %s.\n", p_dev);
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }

  char *p_dev_ip = NULL;
 
  if ((p_dev_ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)) == NULL) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Could not convert IP address of %s.\n", p_dev);
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }
  
  strcpy(p_ip, p_dev_ip);
  
  /* Get MAC address of interface */
  if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Could not retrieve IP address of %s.\n", p_dev);
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }

  char *p_dev_mac = NULL;
  
  if ((p_dev_mac = ether_ntoa((const struct ether_addr *)&ifr.ifr_hwaddr.sa_data[0])) == NULL) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Could not convert IP address of %s.\n", p_dev);
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }  

  strcpy(p_mac, p_dev_mac);

  /* Close socket */
  close(sock);

  if (debug_flag) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "DEVICE INFO: %s %s|%s\n", p_dev, p_ip, p_mac);
    ipwd_message(msgbuf, IPWD_MSG_INFO);
  }

  return(IPWD_RV_SUCCESS);

}
