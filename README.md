# Podział na pliki
- `include/*` pliki nagłówkowe dołączone do zadania
- `cmd` odpowiada za obsługę bloku poleceń:
  - alokację i zwolnienie pamięci dla bloku poleceń
  - wysyłanie poleceń
- `colormaps` odpowiada za obsługę map kolorów
  - tworzenie deskryptorów typu `colormaps`
  - sprawdzanie czy deskryptor jest deskryptorem map kolorów
  - ekstraktowanie danych map kolorów z deskryptora
  - wybór konkretnej mapy kolorów z deskryptora map
- `device` odpowiada za urządzenie znakowe
  - tworzenie/usuwanie urządzenie
  - implementuje operacje na urządzeniu znakowym
- `flat` odpowiada za obsługę tekstur płaskich:
  - tworzenie deskryptorów typu `flat`
  - sprawdzanie czy deskryptor jest deskryptorem tekstury płaskiej
  - ekstraktowanie danych tekstury płaskiej z deskryptora
- `module` implementuje funkcje rejestracji i wyrejestrowania modułu
- `pci` odpowiada za urządzenie pci:
  - inicjalizuje/uwalni prywatne struktury danych
  - tworzy/usuwa urządzenie znakowe
  - włącza/wyłącza urządzenie (ładuje mikrokod itp.)
  - rejestruje obsługę przerwań
- `private` opisuje strukturę danych prywatnych dla danego urządzenia, używaną
 przez wszystkie obiekty stworzone na danym urządzeniu. Zawiera mechanizmy
 synchronizujące, wskażniki dostępu do urządzenia i cache rejestrów urządzenia.
- `pt` funkcje do obsługi tablic stron:
  - znajdowanie długości tablicy stron dla danego bufora
  - inicjalizacjia tablicy stron odpowiednimi wpisami
- `select` zbiór instrukcji do obsługi wyboru ramki/tekstur/map kolorów.
- `surface` odpowiada za obsługę buforów ramki
  - tworzenie deskryptorów typu `surface`
  - sprawdzanie czy deskryptor jest deskryptorem bufora ramki
  - ekstraktowanie danych bufora ramki z deskryptora
  - implementuje operacje na buforze ramki
- `texture` odpowiada za obsługę tekstur kolumnowych:
  - tworzenie deskryptorów typu `texture`
  - sprawdzanie czy deskryptor jest deskryptorem tekstury kolumnowej
  - ekstraktowanie danych tekstury kolumnowej z deskryptora
- `validate` sprawdzanie argumentów poleceń rysowania

# Przydzielanie numerów urządzeń
Numer major jest alokowany dynamicznie przez jądro (`alloc_chrdev_region`).
Do przydzielania kolejnych numerów została użyta mapa bitowa. Z uwagi na
ograniczenie na liczbę urządzeń w systemie (256) jest to bardzo efektywne
rozwiązanie (wystarczają 32 bajty).

# Blok poleceń i synchronizacja
Aby zapewnić spójność wysyłanych poleceń (np. `DRAW` musi zostać wykonane
bezpośrednio po ustawieniu parametrów tego rysowania) każde polecenie ioctl
przed rozpoczęciem wysyłania poleceń musi zdobyć mutex (`doom_prv.cmd_mutex`).
Jest on zwalniany dopiero po wysłaniu wszystkich poleceń w danym wywołaniu.
Aby uzyskać lepsza wydajność oczekiwanie na miejsce w kolejce sprawdzane jest
dla całych pakietów poleceń a wskaźnik `WRITE_PTR` na urządzeniu jest przesuwany
dopiero po wstawienu do kolejki całego pakietu np.:
```c
// surface.c:163
_MUST(cmd_wait(prv->drvdata, 5));
_MUST(cmd(prv->drvdata, HARDDOOM_CMD_FILL_COLOR(line.color)));
_MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_A(line.pos_a_x, line.pos_a_y)));
_MUST(cmd(prv->drvdata, HARDDOOM_CMD_XY_B(line.pos_b_x, line.pos_b_y)));
_MUST(cmd(prv->drvdata, HARDDOOM_CMD_DRAW_LINE));
cmd_commit(prv->drvdata);
```
Najpierw (`cmd_wait`) oczekujemy aż w kolejce będzie __5 wolnych miejsc__ (jedno
na zapas, gdyby była potrzeba wysłania polecenia `FENCE`), następnie do kolejki
wstawiane są polecenia (`cmd`), na koniec przesuwany jest wskażnik `WRITE_PTR`
(`cmd_commit`). `WRITE_PTR` nie jest nigdy czytany z urządzenia, wystarczy
przechowywany w `doom_prv.cmd_idx` aktualny indeks w kolejce.

# Read
Synchronizacja w read została zrealizowana z pomocą mechanizmu `FENCE` oraz
kolejki oczekiwania. Po każdej operacji `ioctl` rysującej na buforze ramki
wysyłane jest polecenie `FENCE` z kolejnymi liczbami przechowywanymi
`doom_prv.fence`. Gdy proces chce czytać dane pobiera aktualny numer `fence` i
czeka na przerwanie. Aby uniknąć wyścigu (proces wysyła wait i czeka, ale zanim
wykona sie przerwanie kolejne zapytanie zwiększa rejestr wait itd. więc przerwanie
nigdy nie nadchodzi) użyto `wait_event_interruptible_timeout`, z częstotliwością
_HZ / 10_ (100 ms), po których ponownie zostanie sprawdzony warunek.

# Cache
Wiele parametrów urządzenia jest cache'owancyh w strukturze `doom_prv`, pozwalając
uniknąć wysyłania niepotrzebnych poleceń i zwiększając wydajność np.:
```c
// select.c:10
static int select_surface_src(struct surface_prv *surface) {
  int err;

  if (surface->drvdata->surf_src == surface->surface_dma)
    return 0;

  _MUST(cmd(surface->drvdata, HARDDOOM_CMD_SURF_SRC_PT(surface->pt_dma)));
  surface->drvdata->surf_src = surface->surface_dma;

  return 0;
}
```
Sprawdzanie cache pozwala uniknąć wysłania polecenia `SURF_SRC_PT` i wyczyszczenia
bufora TLB w przypadku gdy bufor ramki jest już wybrany.
