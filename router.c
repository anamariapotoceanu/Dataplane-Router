
//Potoceanu Ana-Maria 321CA
//Tema 1 - Dataplane Router
#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include "lib.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/byteorder.h>
#include <arpa/inet.h>
#define MAX_ROUTE 100000
#define MAC_LEN 6
#define IP htons(0x0800)
#define ID htons(1)
#define ECHO_REQ 8
#define IPv4 0x0800

struct route_table_entry *route_table;
int route_table_size;

struct arp_table_entry *arp_table;
int arp_size;
struct packet_send {
    int interface;
    char* buf;
    int len;
};
 
//Functie care ajuta la sortarea tabelei de rutare
//Tabela va fi sortata prima data dupa prefix, apoi dupa masca
int compare_function(const void *a, const void *b)
{ 
    struct route_table_entry *entry1, *entry2;
    entry1 = (struct route_table_entry *) a;
    entry2 = (struct route_table_entry *) b;
    uint32_t address1, address2;
    uint32_t mask1, mask2;
    
    mask1 = ntohl(entry1->mask);
    mask2 = ntohl(entry2->mask);
    address1 = ntohl(entry1->prefix) & ntohl(entry1->mask);
    address2 = ntohl(entry2->prefix) & ntohl(entry2->mask);

    if (address1 == address2) {
        if (mask1 == mask2) {
            return 0;
        }
        if (mask1 < mask2) {
            return -1;
        } else if (mask1 > mask2) {
            return 1;
        } 
    }
    if (address1 != address2) {
        if (address1 < address2) {
            return -1;
        } else if (address1 > address2) {
            return 1;
        } 
    }
    return 0;
}

//Functie care returneaza indexul celei mai bune rute pe care o gasim in tabela
int binarysearch_route(struct iphdr *ip_header) {

    int best_index, left, right, middle;
    uint32_t dest;
    dest = ip_header->daddr;
    uint32_t best_mask;
    best_mask = 0;
    best_index = -1;
    left = 0;
    right = route_table_size - 1;

    while (left <= right) {
        middle = left + (right - left) / 2;
        struct route_table_entry *route_mid;
        route_mid = &route_table[middle];
        uint32_t prefix, mask;
        prefix = route_mid->prefix;
        mask = route_mid->mask;
        //In cazul in care gasim potrivirea in tabela, retinem indexul si masca
        if ((ntohl(dest) & ntohl(mask)) == (ntohl(prefix) & ntohl(mask))) {
            //Vom alege potrivirea pentru cea mai buna masca
            if (mask > best_mask) {
                best_index = middle;
                best_mask = mask;
                left = middle + 1;
            }
        } else if (ntohl(dest) < ntohl(prefix)) {
            right = middle - 1;
        } else {
            left = middle + 1;
        }
    }

    return best_index;
}

//Daca gasim adresa IP corespunzatoare urmatorului hop, vom returna indexul
//Aceasta functie ajuta la gasirea adresei MAC corespunzatoare urmatorului hop
int arp_find_index(int index_routetable)
{
    int index_arp, i;
    index_arp = -1;
    for (i = 0; i < arp_size; i++) {
        if (arp_table[i].ip == route_table[index_routetable].next_hop)  { 
            index_arp = i;
        }
    }
    return index_arp;

}

int verify_ttl(char *buf) 
{
    char  *ip_header_start = buf + sizeof(struct ether_header);
    struct iphdr *ip_header = (struct iphdr *)ip_header_start; 
    if (ip_header->ttl <= 1) {//Pachetul nu trebuie trimis mai departe
        return 0;
    }
    return 1;
}

void packet_icmp (int len, char *buf, struct ether_header *eth_hdr,  int interface,
                  struct icmphdr *icmp_header, struct iphdr *ip_header) 
{
    uint16_t new_sum_icmp, new_sum_ip;
    int len_packet;
    len_packet = sizeof (struct iphdr) + sizeof(struct icmphdr);

    //Inainte de trimiterea pachetului, trebuie recalculata suma din antet
    icmp_header->checksum = 0;
    new_sum_icmp = checksum((uint16_t *) icmp_header, sizeof(struct icmphdr));
    icmp_header->checksum = htons(new_sum_icmp);

    eth_hdr->ether_type = IP;
    //Folosim o copie a antetului
    //Noua sursa va fi vechea destinatie, iar vechea destinatie este sursa
    struct ether_header ether_header_copy;
    memcpy(&ether_header_copy, eth_hdr, sizeof(struct ether_header));
    memcpy(eth_hdr->ether_dhost, ether_header_copy.ether_shost, MAC_LEN);
    memcpy(eth_hdr->ether_shost, ether_header_copy.ether_dhost, MAC_LEN);

    ip_header->id = ID;
    ip_header->tot_len = htons(len_packet);
    //Folosim o copie pentru a retine adresele initiale
    struct iphdr ip_header_copy;
    memcpy(&ip_header_copy, ip_header, sizeof(struct iphdr));
    ip_header->daddr = ip_header_copy.saddr;
    ip_header->saddr = ip_header_copy.daddr;

    ip_header->check = 0;
    new_sum_ip = checksum((uint16_t *) ip_header, sizeof(struct iphdr));
    ip_header->check = htons(new_sum_ip);
    //Pachetul care va fi trimis va avea lungimea pachetului initial, la care
    //se adauga dimensiunea structurii struct icmphdr
    struct packet_send new_packet = {.interface = interface, .buf = buf, .len = len + sizeof(struct icmphdr)};

    send_to_link(new_packet.interface, new_packet.buf, new_packet.len);

}

void function_icmp(int len, char *buf, struct ether_header *eth_hdr,  uint8_t type, int interface,
                    struct iphdr *ip_header) 
{
    void  *icmp_header_start = buf + sizeof(struct ether_header) + sizeof(struct iphdr);
    struct icmphdr *icmp_header = (struct icmphdr *)icmp_header_start;
    //Atunci cand se trimite un mesaj de eroare, avem trei cazuri
    switch (type) {
        case 3://Nu a fost gasita destinatia
            ip_header->protocol = 1;
            icmp_header->type = type;
            icmp_header->code = 0;
            packet_icmp(len, buf, eth_hdr, interface, icmp_header, ip_header);
            break;
        case 0://Routerul primeste mesaje destinate lui insusi
            ip_header->protocol = 1;
            icmp_header->type = type;
            icmp_header->code = 0;
            packet_icmp(len, buf, eth_hdr, interface, icmp_header, ip_header);
            break;
        case 11://TTL a expirat
            ip_header->protocol = 1;
            icmp_header->type = type;
            icmp_header->code = 0;
            packet_icmp(len, buf, eth_hdr, interface, icmp_header, ip_header);
            break;
        default:
            printf("Nu avem packet icmp\n");
    }
}


int verify_sum(char *buf)
{
    uint16_t sum, new_sum; 
    char  *ip_header_start = buf + sizeof(struct ether_header);
    struct iphdr *ip_header = (struct iphdr *)ip_header_start;

    sum = ip_header->check;
    sum = ntohs(sum);
    ip_header->check = 0;
    new_sum = checksum((uint16_t *)ip_header, sizeof(struct iphdr));

    if (sum != new_sum) 
        return 0;
    return 1;
}

int verify_destination_icmp(char *buf, int interface, struct iphdr *ip_header)
{
    uint32_t router_address;
    char *icmp_header_start = buf + sizeof(struct ether_header) + sizeof(struct iphdr);
    struct icmphdr *icmp_header = (struct icmphdr *)icmp_header_start;
    char *router_ip = get_interface_ip(interface);
    router_address = inet_addr(router_ip);
    //Verificam daca destinatia finala este routerul
    //Routerul poate primi mesaje ICMP de tip "Echo request"
    if ((ip_header->daddr == router_address) && (icmp_header->type == ECHO_REQ)) {
        return 1;
    }
    return 0;
}

void function_ipv4(char *buf, size_t len, int interface, struct ether_header *eth_hdr)
{
    uint8_t addr_source[MAC_LEN];
    int index_binary, index_arp;
    uint16_t new_sum;
    char  *ip_header_start = buf + sizeof(struct ether_header);
    struct iphdr *ip_header = (struct iphdr *)ip_header_start; 
    
    //Daca destinatia finala este insusi routerul, pachetul nu este trimis
    if (verify_destination_icmp(buf, interface, ip_header) == 1) {
        function_icmp(len, buf, eth_hdr,  0, interface, ip_header) ;
        return;
    }

    //Verificare daca suma corespunde cu suma primita in antet
    if (verify_sum(buf) == 0)
        return;
	
    if (verify_ttl(buf) == 0) {
        function_icmp (len, buf, eth_hdr,  11, interface, ip_header) ;
        return;
    }
    //Daca putem trimite pachetul, atunci trebuie sa decrementam TTL
    if (verify_ttl(buf) == 1) {
        ip_header->ttl -= 1;
    }

    //Cautam cea mai buna ruta, folosind Binary Search
    index_binary = binarysearch_route(ip_header);

    //Daca nu gasim ruta, atunci va trebui sa trimitem un mesaj de eroare ICMP
    if (index_binary == -1) {
        function_icmp(len, buf, eth_hdr, 3, interface, ip_header) ;
        return; 
    }

    //Cautam in tabela ARP, adresa MAC corespunzatore urmatorului hop
    index_arp = arp_find_index(index_binary);

    //Inainte de a trimite pachetul, trebuie sa recalculam suma
    ip_header->check = 0;
    new_sum = checksum((uint16_t *) ip_header, sizeof(struct iphdr));
    ip_header->check = htons(new_sum);

    //Adresa MAC sursa va fi cea corespunzatoare interfetei routerului
    get_interface_mac(route_table[index_binary].interface, addr_source);
    memcpy(eth_hdr->ether_shost, addr_source, MAC_LEN);

    //Adresa MAC destinatie o aflam din tabela ARP
    memcpy(eth_hdr->ether_dhost, arp_table[index_arp].mac, MAC_LEN);
    struct packet_send new_packet = {.interface = route_table[index_binary].interface, .buf = buf, .len = len + sizeof(struct icmphdr)};

    send_to_link(new_packet.interface, new_packet.buf, new_packet.len);

}


int main(int argc, char *argv[])
{
    char buf[MAX_PACKET_LEN];

    // Do not modify this line
    init(argc - 2, argv + 2);

    route_table = malloc(sizeof(struct route_table_entry) * MAX_ROUTE);
    arp_size = 0;
    arp_table = malloc(sizeof(struct arp_table_entry) * MAX_ROUTE);

    route_table_size = read_rtable(argv[1], route_table);
    qsort(route_table, route_table_size, sizeof(struct route_table_entry), compare_function);
    arp_size = parse_arp_table("arp_table.txt", arp_table);

    while (1) {

        int interface;
        size_t len;

        interface = recv_from_any_link(buf, &len);
        DIE(interface < 0, "recv_from_any_links");

        struct ether_header *eth_hdr = (struct ether_header *) buf;    
        if (eth_hdr->ether_type == IP) {
            function_ipv4(buf, len, interface, eth_hdr);
        }
       
    }
    free(route_table);
    free(arp_table);
}
