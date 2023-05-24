Tema 2- Loader de executabile

Rezolvarea temei se bazeaza pe implementarea functiei segv_handler, pentru tratarea evenimentelor
de tip SEGMENTATION FAULT.
Prin apelarea acestei functii, se verifica toate segmentele executabilului si se determina cel la 
nivelul caruia s-a produs SEGMENTATION FAULT. Calculam numarul paginii din segment si verificam 
maparea acesteia in memorie cu ajutorul identificatorului SEGV_MAPERR.

In cazul in care pagina nu este mapata in memorie(==SEGV_MAPERR), cu ajutorul functiilor mmap, copy_into
si read_from se realizeaza maparea.

copy_into(so_seg_t *segment, void *page_address, int nr_page): Copierea instructiunilor 
la adresa page_address
Cu mprotect, setam permisiunile corespunzatoare segmentului.

read_from(char *buffer, int size): citirea in buffer a maxim size bytes
(dimensiunea pagini/cat mai avem de citit din fisier)


In alta situatie, se apeleaza handler-ul default.
