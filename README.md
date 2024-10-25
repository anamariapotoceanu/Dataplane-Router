## Tema 1 - Dataplane Router


* Se verifica tipul pachetului primit. Daca se constanta ca este de tipul IPv4,
atunci se apeleaza functia corespunzatore (function_ipv4).
- In cadrul acestei functii, initial am verificat daca pachetul are ca
destinatie routerul si daca primeste un mesaj destinat lui (type 8 pentru
ICMP). In acest caz, routerul nu trimite pachetul mai departe, ci va incerca
sa-l proceseze (verify_destination_icmp). 
- Urmatorul pas a fost verificarea sumelor. In cazul in care suma difera de cea
primita in antet, inseamna ca pachetul a fost corupt, deci nu va fi trimis mai
departe.
- Pentru TTL, am verificat daca este mai mic sau egal decat 0, adica daca nu
poate fi trimis mai departe. Altfel, am decrementat TTL. 
- Mai departe, am cautat in tabela de rutare, adresa urmatorului hop. Daca se
constata ca nu avem ruta, se trimite un mesaj de tipul ICMP, type 3. Pentru
cautarea in tabela de rutare, am folosit metoda binary search. Pentru a face
eficienta cautarea, prima data am sortat tabela de rutare in functie de prefix
si de masca.
- Daca reusim sa gasim adresa urmatorului hop, cautam in tabela arp, adresa MAC
corespunzatoare pentru a putea trimite pachetul. 
- Inainte de trimiterea pachetului, ne asiguram ca suma este recalculata.
Se seteaza adresele MAC sursa si destinatie. Se trimite pachetul pe interfata
urmatorului hop gasit anterior.

* Pentru trimiterea pachetelor de tipul ICMP, am creat o functie function_icmp,
care in functie de tipul mesajului, va seta tipul corespunzator si codul
specific. Tipul 3 este corespunzator cazului cand nu se gaseste destinatia,
tipul 0 este atunci cand routerul primeste un mesaj si trebuie sa-si raspunda
lui insusi, iar ultimul tip, 11 este pentru TTL. Pentru fiecare pachet ICMP, se
va seta in antentul ICMP tipul si codul. De asemenea, campul protocol din
antetul IP este dedicat tot mesajelor ICMP. 
* Pentru trimiterea pachetelor, este necesara construirea celor trei anteturi.
Destinatia va fi noua sursa, iar vechea sursa este noua destinatie. Pentru a
retine aceste aspecte, m-am folosit de copii ale anteturilor ether_header_copy
si ip_header_copy. De asemenea, am recalculat sumele pentru cele doua anteturi.
Lungimea pachetului care va fi trimis, va fi lungimea pachetului initial la
care se va adauga dimensiunea structurii struct icmphdr. Pentru trimitea
pachetului, am creat un nou pachet cu continutul, interfata si lungimea
necesara.
