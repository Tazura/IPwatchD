/* IPwatchD - IP conflict detection tool for Linux
 * Copyright (C) 2007 Jaroslav Imrich <jariq(at)jariq(dot)sk>
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


extern int    debug_flag;               /* Flag indicating debug mode */
extern char   msgbuf[IPWD_MSG_BUFSIZ];  /* Buffer for output messages */


/* ipwd_genarp - Generate ARP packet with libnet1
 * 
 * Parameters:
 *   - dev    - name of the device
 *   - p_sip  - source IP address
 *   - p_smac - source MAC address
 *   - p_dip  - destination IP address
 *   - p_dmac - destination MAC address
 *   - opcode - ARPOP_REQUEST or ARPOP_REPLY
*/

int ipwd_genarp(char *dev, char *p_sip, char *p_smac, char *p_dip, char *p_dmac, int opcode) {

  struct in_addr sip, dip;
  struct ether_addr smac, dmac;

  /* Convert source IP address */
  if (inet_aton(p_sip, &sip) == 0) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to convert source IP address.\n");
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }
  
  /* Convert destination IP address */
  if (inet_aton(p_dip, &dip) == 0) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to convert destination IP address.\n");
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }

  /* Convert source MAC address */
  if (ether_aton_r(p_smac, &smac) == NULL) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to convert source MAC address.\n");
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }

  /* Convert destination MAC address */
  if (ether_aton_r(p_dmac, &dmac) == NULL) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to convert destination MAC address.\n");
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }
  
  /* Set opcode once again. Just in case.. */
  if (opcode != ARPOP_REQUEST) {
    opcode = ARPOP_REPLY;
  }
  
  libnet_t *h_net = NULL;
  char errbuf[LIBNET_ERRBUF_SIZE];
  
  /* Initialize libnet */
  h_net = libnet_init(LIBNET_LINK_ADV, dev, errbuf);
  if (h_net == NULL) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to initialize libnet1 - %s\n", errbuf);
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    return(IPWD_RV_ERROR);
  }

  /* Gratuitous ARP request will be created if destination MAC address is broadcast.
     GARP requests from Windows and OpenBSD have destination MAC in ARP header
     always set to 00:00:00:00:00:00 so we will do the same thing */
  if ((strcmp(p_dmac, "ff:ff:ff:ff:ff:ff") == 0) && (opcode == ARPOP_REQUEST)) {

    struct ether_addr nullmac;
    char *null_mac = "00:00:00:00:00:00";

    /* Convert null MAC address */
    if (ether_aton_r(null_mac, &nullmac) == NULL) {
      snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to convert destination MAC address for GARP request.\n");
      ipwd_message(msgbuf, IPWD_MSG_ERROR);
      libnet_destroy(h_net);
      return(IPWD_RV_ERROR);
    }

    /* Build ARP header for gratuitous ARP packet */
    libnet_ptag_t arp = 0;
    arp = libnet_build_arp(ARPHRD_ETHER, ETHERTYPE_IP, 6, 4, opcode, (u_int8_t *) &smac, (u_int8_t *) &sip, (u_int8_t *) &nullmac, (u_int8_t *) &dip, NULL, 0, h_net, arp);
    if (arp == -1) {
      snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to build ARP header: %s\n", libnet_geterror(h_net));
      ipwd_message(msgbuf, IPWD_MSG_ERROR);
      libnet_destroy(h_net);
      return(IPWD_RV_ERROR);
    }
    
  } else {

    /* Build ARP header for normal ARP packet */
    libnet_ptag_t arp = 0;
    arp = libnet_build_arp(ARPHRD_ETHER, ETHERTYPE_IP, 6, 4, opcode, (u_int8_t *) &smac, (u_int8_t *) &sip, (u_int8_t *) &dmac, (u_int8_t *) &dip, NULL, 0, h_net, arp);
    if (arp == -1) {
      snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to build ARP header: %s\n", libnet_geterror(h_net));
      ipwd_message(msgbuf, IPWD_MSG_ERROR);
      libnet_destroy(h_net);
      return(IPWD_RV_ERROR);
    }

  }

  /* Build ethernet header */
  libnet_ptag_t ether = 0;
  ether = libnet_build_ethernet((u_int8_t *) &dmac, (u_int8_t *) &smac, ETHERTYPE_ARP, NULL, 0, h_net, ether);
  if (ether == -1) {
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to build ethernet header: %s\n", libnet_geterror(h_net));
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    libnet_destroy(h_net);
    return(IPWD_RV_ERROR);
  }

  /* Send packet */
  int c = libnet_write(h_net);
  if (c == -1) {
    
    snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Unable to send packet: %s\n", libnet_geterror(h_net));
    ipwd_message(msgbuf, IPWD_MSG_ERROR);
    libnet_destroy(h_net);
    return(IPWD_RV_ERROR);
    
  } else {
    
    if (debug_flag) {
      snprintf(msgbuf, IPWD_MSG_BUFSIZ, "Packet with size of %d bytes sent.\n", c);
      ipwd_message(msgbuf, IPWD_MSG_ERROR);
    }
    
  }

  libnet_destroy(h_net);

  return(IPWD_RV_SUCCESS);

}
