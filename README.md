## Dataplane Router

- Se verifică tipul pachetului primit. Dacă se constată că este de tipul IPv4, atunci se apelează funcția corespunzătoare (`function_ipv4`).
  
- În cadrul acestei funcții, inițial am verificat dacă pachetul are ca destinație routerul și dacă primește un mesaj destinat lui (type 8 pentru ICMP). În acest caz, routerul nu trimite pachetul mai departe, ci va încerca să-l proceseze (`verify_destination_icmp`).

- Următorul pas a fost verificarea sumelor. În cazul în care suma diferă de cea primită în antet, înseamnă că pachetul a fost corupt, deci nu va fi trimis mai departe.

- Pentru TTL, am verificat dacă este mai mic sau egal decât 0, adică dacă nu poate fi trimis mai departe. Altfel, am decrementat TTL.

- Mai departe, am căutat în tabela de rutare adresa următorului hop. Dacă se constată că nu avem rută, se trimite un mesaj de tipul ICMP, type 3. Pentru căutarea în tabela de rutare, am folosit metoda binary search. Pentru a face căutarea eficientă, prima dată am sortat tabela de rutare în funcție de prefix și de mască.

- Dacă reușim să găsim adresa următorului hop, căutăm în tabela ARP adresa MAC corespunzătoare pentru a putea trimite pachetul.

- Înainte de trimiterea pachetului, ne asigurăm că suma este recalculată. Se setează adresele MAC sursă și destinație. Se trimite pachetul pe interfata următorului hop găsit anterior.

---

- Pentru trimiterea pachetelor de tip ICMP, am creat o funcție `function_icmp`, care, în funcție de tipul mesajului, va seta tipul corespunzător și codul specific. 
  - Tipul 3 este corespunzător cazului când nu se găsește destinația.
  - Tipul 0 este atunci când routerul primește un mesaj și trebuie să-și răspundă lui însuși.
  - Tipul 11 este pentru TTL.
  
  Pentru fiecare pachet ICMP, se va seta în antetul ICMP tipul și codul. De asemenea, câmpul protocol din antetul IP este dedicat tot mesajelor ICMP.

---

- Pentru trimiterea pachetelor, este necesară construirea celor trei anteturi. Destinația va fi noua sursă, iar vechea sursă este noua destinație. 
  - Pentru a reține aceste aspecte, m-am folosit de copii ale anteturilor `ether_header_copy` și `ip_header_copy`.
  - De asemenea, am recalculat sumele pentru cele două anteturi. 
  - Lungimea pachetului care va fi trimis va fi lungimea pachetului inițial la care se va adăuga dimensiunea structurii `struct icmphdr`.
  
- Pentru trimiterea pachetului, am creat un nou pachet cu conținutul, interfața și lungimea necesară.
