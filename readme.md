# PinguQueen

----


### Idee

Idee des Projektes ist ein CLI-Tool für Linux-Distributionen, realisiert im Frontend mit FTXGUI, dass beim erstmaligen Programmaufruf die Ordner- und Dateistrukturen
in einer komprimierten Binärdatei speichert und in einem Adaptive-Radix-Trie(https://db.in.tum.de/~leis/papers/ART.pdf) abbildet. Dies soll darauffolgend zu ultra-super-krass schnellen Dateisuchen führen.

**Zusätzlich/Optional** könnten Live-Veränderungen an dem Dateisystem mit inotify abgefangen und in dem Trie angepasst werden.

Dieses Projekt ist eine wilde Mischung aus richtig hässlichem Data-Oriented-Design in den Datenstrukturen und hoffentlich schöner Objektorientierung im File-Handling und Frontend.



Unter der executable "All CTest" können die Testtreiber zu dem Adaptive-Radix-Trie gestartet werden und unter
"radix-trie-benchmarks", die benchmarktests für den ART.

Beide diese Tests, sind bis jetzt KI generiert.
