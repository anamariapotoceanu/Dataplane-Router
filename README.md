## Tema 1 - Dataplane Router

- **Verificarea tipului pachetului**: Se verifică tipul pachetului primit. Dacă se constată că este de tipul IPv4, se apelează funcția corespunzătoare (`function_ipv4`).
  
- **Procesarea pachetului IPv4**:
  - În cadrul acestei funcții, inițial am verificat dacă pachetul are ca destinație routerul și dacă primește un mesaj destinat lui (type 8 pentru ICMP). În acest caz, routerul nu trimite pachetul mai departe, ci va încerca să-l proceseze (`verify_destination_icmp`).
  - Următorul pas a fost verificarea sumelor. Dacă suma diferă de cea primită în antet, înseamnă că pachetul a fost corupt și nu va fi trimis mai departe.
  - Pentru TTL, am verificat dacă este mai mic sau egal cu 0, ceea ce înseamnă că pachetul nu poate fi trimis mai departe. Altfel, am decrementat TTL-ul.
  - Am căutat în tabela de rutare adresa următorului hop. Dacă nu avem rută, se trimite un mesaj ICMP de tip 3. Am folosit metoda de căutare binară pentru a găsi ruta; prima dată am sortat tabela de rutare în funcție de prefix și mască.
  - Dacă reușim să găsim adresa următorului hop, căutăm în tabela ARP adresa MAC corespunzătoare pentru a putea trimite pachetul.
  - Înainte de a trimite pachetul, ne asigurăm că suma este recalculată, setăm adresele MAC sursă și destinație și trimitem pachetul pe interfata următorului hop găsit anterior.

- **Trimiterea pachetelor ICMP**: Am creat o funcție (`function_icmp`) care, în funcție de tipul mesajului, va seta tipul corespunzător și codul specific:
  - Tipul 3 corespunde cazului când nu se găsește destinația.
  - Tipul 0 este pentru mesajul primit de router care trebuie să-și răspundă lui însuși.
  - Tipul 11 este pentru TTL.
  
  Pentru fiecare pachet ICMP, se va seta în antetul ICMP tipul și codul, iar câmpul protocol din antetul IP este dedicat mesajelor ICMP.

- **Construirea anteturilor**: Pentru trimiterea pachetelor, este necesară construirea celor trei anteturi. Destinația va fi noua sursă, iar vechea sursă va deveni noua destinație. Am utilizat copii ale anteturilor `ether_header_copy` și `ip_header_copy` pentru a reține aceste aspecte și am recalculat sumele pentru cele două anteturi. 
  - Lungimea pachetului care va fi trimis este lungimea pachetului inițial la care se adaugă dimensiunea structurii `struct icmphdr`. 
  - În final, am creat un nou pachet cu conținutul, interfata și lungimea necesară pentru trimiterea pachetului.
