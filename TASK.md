# Zadanie 2: sterownik urządzenia HardDoom™

__Data ogłoszenia:__ 10.04.2018
__Termin oddania:__ 15.05.2018 (ostateczny 29.05.2018)

## Materiały dodatkowe

- Symulator urządzenia: https://github.com/koriakin/qemu (branch `harddoom`)
- Program testowy: https://github.com/koriakin/prboom-plus (branch `doomdev`)
- Plik nagłówkowy z definicjami rejestrów sprzętowych (do skopiowania do
  rozwiązania):
  https://github.com/koriakin/qemu/blob/harddoom/hw/misc/harddoom.h
- Plik nagłówkowy z definicjami interfejsu urządzenia znakowego (do
  skopiowania do rozwiązania):
  https://github.com/koriakin/prboom-plus/blob/doomdev/src/doomdev.h
- Mikrokod potrzebny do uruchomienia urządzenia:
  https://github.com/koriakin/harddoom/blob/master/doomcode.bin
  lub https://github.com/koriakin/harddoom/blob/master/doomcode.h

## Wprowadzenie

Zadanie polega na napisaniu sterownika do __urządzenia HardDoom™__,
będącego akceleratorem grafiki dedykowanym dla gry Doom. Urządzenie
dostarczane jest w postaci [zmodyfikowanej wersji qemu](#qemu).

Urządzenie powinno być dostępne dla użytkownika w formie urządzenia
znakowego. Dla każdego urządzenia HardDoom™ obecnego w systemie należy
utworzyć urządzenie znakowe `/dev/doomX`, gdzie X to numer kolejny
urządzenia HardDoom™, zaczynając od 0.

## Interfejs urządzenia znakowego

Urządzenie `/dev/doom*` służy tylko i wyłącznie do tworzenia zasobów
HardDoom™ - wszystkie właściwe operacje będą wykonywane na utworzonych
zasobach. Powinny one obsługiwać następujące operacje:

- `open`: w oczywisty sposób
- `close`: w oczywisty sposób
- `ioctl(DOOMDEV_IOCTL_CREATE_SURFACE)`: tworzy nowy bufor ramki na
  urządzeniu. Jako parametr tego wywołania przekazywane są wymiary bufora
  (szerokość i wysokość).  Szerokość musi być wielokrotnością 64 z zakresu
  _64 .. 2048_, a wysokość musi być w zakresie _1 .. 2048_.  Wynikiem tego
  wywołania jest nowy deskryptor pliku odnoszący się do utworzonego bufora.
  Utworzony bufor ma nieokreśloną zawartość.
- `ioctl(DOOMDEV_IOCTL_CREATE_TEXTURE)`: tworzy nową teksturę kolumnową na
  urządzeniu. Parametrami tego wywołania są rozmiar tekstury w bajtach
  __(maksymalnie 4MiB)__, wysokość tekstury w tekselach (maksymalnie 1023,
  lub 0, jeśli tekstura nie ma być powtarzana w pionie), oraz wskaźnik na
  dane tekstury. Wynikiem jest deskryptor pliku odnoszący się do utworzonej
  tekstury.
- `ioctl(DOOMDEV_IOCTL_CREATE_FLAT)`: tworzy nową teksturę płaską na
  urządzeniu. Parametrem tego wywołania jest wskaźnik na dane (_0x1000_
  bajtów). Wynikiem jest deskryptor pliku odnoszący się do utworzonej tekstury.
- `ioctl(DOOMDEV_IOCTL_CREATE_COLORMAPS)`: tworzy nową tablicę map kolorów na
  urządzeniu. Parametrami tego wywołania są rozmiar tablicy (liczba map
  kolorów) oraz wskaźnik na dane (każda mapa kolorów to _0x100_ bajtów).
  Wynikiem jest deskryptor pliku odnoszący się do utworzonej tablicy.
  Maksymalny dopuszczalny rozmiar to tablica _0x100_ map.

Tekstury i tablice map kolorów nie obsługują żadnych standardowych operacji
poza `close` (która, o ile nie zostały już żadne inne referencje, zwalnia ich
pamięć) - można ich użyć jedynie jako parametrów do wywołań rysowania. Nie da
się również w żaden sposób zmienić ich zawartości po utworzeniu.

Wszystkie wskaźniki przekazywane są jako `uint64_t`, aby struktury miały taki
sam układ w trybie 64-bitowym jak w 32-bitowym, pozwalając na uniknięcie
konieczności definiowania odpowiedających struktur `_compat`.  Z tego samego
powodu wiele struktur ma nieużywane pola `_pad`.

Na buforach ramki można wywołać następujące operacje:

- `ioctl(DOOMDEV_SURF_IOCTL_COPY_RECT)`: wykonuje serię operacji `COPY_RECT`
  do danego bufora. Parametry to:

  - `surf_src_fd`: deskryptor pliku wskazujący na bufor ramki, z którego ma być
    wykonana kopia.
  - `rects_ptr`: wskaźnik na tablicę struktur `doomdev_copy_rect`.
  - `rects_num`: liczba prostokątów do skopiowania.
  - w każdej strukturze `doomdev_copy_rect`:

    - `pos_dst_x`, `pos_dst_y`: współrzędne prostokąta docelowego w danym
      buforze (lewy górny róg).
    - `pos_src_x`, `pos_src_y`: współrzędne prostokąta źródłowego w buforze
      źródłowym.
    - `width`, `height` - rozmiar kopiowanego prostokąta.

  Semantyka tego wywołania jest dość podobna do `write`: sterownik stara się
  wykonać jak najwięcej operacji z zadanej listy, zatrzymując się w razie błędu
  bądź przyjścia sygnału. Jeśli nie udało się wykonać pierwszej operacji,
  zwracany jest kod błędu. W przeciwnym przypadku, zwracana jest liczba
  wykonanych operacji. Kod użytkownika jest odpowiedzialny za ponowienie próby
  w przypadku niepełnego wykonania.

  Użytkownik jest odpowiedzialny, za zapewnienie, że w ramach jednego wywołania
  `ioctl` żaden piksel nie jest jednocześnie pisany i czytany (tzn. nie będzie
  wymagane polecenie `INTERLOCK` między prostokątami). Sterownik nie musi (choć
  jak bardzo chce to może) wykrywać takich sytuacji - wysłanie poleceń na
  urządzenie i uzyskanie błędnego wyniku rysowania jest dopuszczalne w takiej
  sytuacji.

- `ioctl(DOOMDEV_SURF_IOCTL_FILL_RECT)`: wykonuje serię operacji `FILL_RECT`.
  Parametry:

  - `rects_ptr`: wskaźnik na tablicę struktur `doomdev_fill_rect`.
  - `rects_num`: liczba prostokątów do wypełnienia.
  - w każdej strukturze `doomdev_fill_rect`:

    - `pos_dst_x`, `pos_dst_y`: współrzędne prostokąta docelowego w danym
      buforze.
    - `width`, `height`: rozmiar wypełnianego prostokąta.
    - `color`: kolor, którym będzie wypełniany prostokąt.

  Wartość zwracana jak przy `DOOMDEV_SURF_IOCTL_COPY_RECT`.

- `ioctl(DOOMDEV_SURF_IOCTL_DRAW_LINE)`: wykonuje serię operacji `DRAW_LINE`.
  Parametry:

  - `lines_ptr`: wskaźnik na tablicę struktur `doomdev_line`.
  - `lines_num`: liczba linii do narysowania.
  - w każdej strukturze `doomdev_line`:

    - `pos_a_x`, `pos_a_y`: współrzędne jednego z końców linii.
    - `pos_b_x`, `pos_b_y`: współrzędne drugiego z końców linii.
    - `color`: kolor, którym będzie rysowana linia.

  Wartość zwracana jak przy `DOOMDEV_SURF_IOCTL_COPY_RECT`.

- `ioctl(DOOMDEV_SURF_IOCTL_DRAW_BACKGROUND)`: wykonuje operację
  `DRAW_BACKGROUND`. Parametry:

  - `flat_fd`: deskryptor pliku wskazujący na teksturę płaską, która będzie
    służyć za tło.

  W przypadku udanego wywołania zwraca 0.

- `ioctl(DOOMDEV_SURF_IOCTL_DRAW_COLUMNS)`: wykonuje serię operacji
  `DRAW_COLUMN` z podobnymi parametrami.  Parametry wspólne dla wszystkich  
  kolumn:

  - `draw_flags`: ustawienia rysowania, kombinacja następujących flag:

    - `DOOMDEV_DRAW_FLAGS_FUZZ`: jeśli włączona, rysowany będzie efekt
      cienia -- większość parametrów jest ignorowana (w tym pozostałe flagi).
    - `DOOMDEV_DRAW_FLAGS_TRANSLATE`: jeśli włączona, paleta będzie
      przemapowana zgodnie z mapą kolorów.
    - `DOOMDEV_DRAW_FLAGS_COLORMAP`: jeśli włączona, kolory będą przyciemnane
      zgodnie z mapą kolorów.

  - `texture_fd`: deksryptor tekstury kolumnowej. Ignorowany, jeśli użyto
    flagi `FUZZ`.
  - `translation_fd`: deskryptor tablicy map kolorów użytej do opcji    
    `TRANSLATE`. Ignorowany, jeśli nie użyto flagi `TRANSLATE`.
  - `colormap_fd`: deskryptor tablicy map kolorów użytej do opcji `COLORMAP`
    oraz `FUZZ`. Ignorowany, jeśli nie użyto żadnej z tych flag.
  - `translation_idx`: indeks mapy kolorów użytej do zmiany palety (używany
    tylko, jeśli flaga `TRANSLATE` jest włączona).
  - `columns_num`: liczba kolumn do rysowania.
  - `columns_ptr`: wskaźnik (w przestrzeni użytkownika) na parametry ustawiane
    osobno dla każdej kolumny, zapisane jako tablica struktur `doomdev_column`:

    - `column_offset`: offset początku danych kolumny w teksturze w bajtach.
    - `ustart`: liczba stałoprzecinkowa 16.16 bez znaku, musi być w zakresie
      obsługiwanym przez sprzęt. Ignorowane, jeśli użyto flagi `FUZZ`.
    - `ustep`: liczba stałoprzecinkowa 16.16 bez znaku, musi być w zakresie
      obsługiwanym przez sprzęt.
    - `x`: współrzędna `x` kolumny.
    - `y1`, `y2`: współrzędne `y` początku i końca koumny.
    - `colormap_idx`: indeks mapy kolorów użytej do przyciemniania kolorów.
      Używany tylko, jeśli flaga `FUZZ` lub `COLORMAP` jest włączona.

  Wartość zwracana jak przy `DOOMDEV_SURF_IOCTL_COPY_RECT`.

- `ioctl(DOOMDEV_SURF_IOCTL_DRAW_SPANS)`: wykonuje serię operacji `DRAW_SPAN`
  z podobnymi parametrami. Parametry wspólne dla wszystkich pasków:

  - `flat_fd`: deksryptor tekstury płaskiej.
  - `translation_fd`: jak wyżej.
  - `colormap_fd`: jak wyżej.
  - `draw_flags`: jak wyżej, ale bez flagi `FUZZ`.
  - `translation_idx`: jak wyżej.
  - `spans_num`: liczba pasków do rysowania
  - `spans_ptr`: wskaźnik (w przestrzeni użytkownika) na dane pasków do
    rysowania, zapisane jako tablica struktur `doomdev_span`:

    - `ustart`, `vstart`: początkowe współrzędne w teksturze (dla lewego
      końca). Liczby stałoprzecinkowe 16.16 (wysokie 10 bitów powinno być
      zignorowane przez sterownik).
    - `ustep`, `vstep`: krok współrzędnych x, y. Format taki jak przy
      `*start`.
    - `x1`, `x2`: współrzędne `x` początku i końca paska.
    - `y`: współrzędna `y` paska.
    - `colormap_idx`: jak wyżej.

  Wartość zwracana jak przy `DOOMDEV_SURF_IOCTL_COPY_RECT`.

- `lseek`: ustawia pozycję w buforze dla następnych wywołań `read`.
- `read`, `pread`, `readv`, itp.: czeka na zakończenie wszystkich wcześniej
  wysłanych operacji rysujących do danego bufora, po czym czyta gotowe dane z
  bufora do przestrzeni użytkownika. W razie próby czytania poza zakresem
  bufora, należy poinformować o końcu pliku.

Sterownik powinien wykrywać polecenia z niepoprawnymi parametrami (zły typ
pliku przekazany jako `*_fd`, współrzędne wystające poza bufor ramki, itp.) i
zwrócić błąd `EINVAL`.  W przypadku próby stworzenia tekstur czy buforów ramki
większych niż obsługiwane przez sprzęt, należy zwrócić `EOVERFLOW`.

Sterownik powinien rejestrować swoje urządzenia w __sysfs__, aby udev
automatycznie utworzył pliki urzadzeń o odpowiednich nazwach w `/dev`.
Numery major i minor dla tych urządzeń są dowolne (majory powinny być
alokowane dynamicznie).

Plik nagłówkowy z odpowiednimi definicjami można znaleźć tutaj:
https://github.com/koriakin/prboom-plus/blob/doomdev/src/doomdev.h

## Założenia interakcji ze sprzętem

Można założyć, że przed załadowaniem sterownika, urządzenie ma stan jak po
resecie sprzętowym. Urządzenie należy też w takim stanie zostawić przy
odładowaniu sterownika.

Pełnowartościowe rozwiązanie powinno działać asynchronicznie - rysujące
operacje `ioctl` powinny wysyłać polecenia do urządzenia i wracać do
przestrzeni użytkownika bez oczekiwania na zakończenie działania (ale jeśli
bufory poleceń są już pełne, dopuszczalne jest oczekiwanie na zwolnienie
miejsca w nich).  Oczekiwanie na zakończenie poleceń powinno być wykonane
dopiero przy wywołaniu `read`, które będzie rzeczywiście potrzebowało
wyników rysowania.

_Implementując sterownik można zignorować funkcje suspend itp. - zakładamy,
że komputer nie będzie usypiany (jest to bardzo zła praktyka, ale niestety
nie mamy możliwości sensownego przetestowania suspendu w qemu ze względu na
bugi w virtio)._

## Zasady oceniania

Za zadanie można uzyskać do 10 punktów. Na ocenę zadania składają się trzy
części:

- pełne wykorzystanie urządzenia (od 0 do 2 punktów):

  - operacja w pełni asynchroniczna (`ioctl` nie czeka na zakończenie
    wysłanych poleceń, rozpoczęcie wysyłania poleceń przez `ioctl`
    nie czeka na zakończenie poleceń wysłanych wcześniej, `read` nie wymaga
    zatrzymania całego urządzenia): 1p
  - użycie bloku wczytywania poleceń: 1p

- wynik testów (od 0 do 8 punktów)
- ocena kodu rozwiązania (od 0 do -10 punktów)

## Forma rozwiązania

Sterownik powinien zostać zrealizowany jako moduł jądra Linux w wersji
4.9.13.  Moduł zawierający sterownik powinien nazywać się `harddoom.ko`.
Jako rozwiązanie należy dostarczyć paczkę zawierającą:

- źródła modułu
- pliki `Makefile` i `Kbuild` pozwalające na zbudowanie modułu
- krótki opis rozwiązania

## QEMU

Do użycia urządzenia HardDoom™ wymagana jest zmodyfikowana wersja qemu,
dostępna w wersji źródłowej.

Aby skompilować zmodyfikowaną wersję qemu, należy:

1. Sklonować repozytorium https://github.com/koriakin/qemu
2. `git checkout harddoom`
3. Upewnić się, że są zainstalowane zależności: `ncurses`, `libsdl`, `curl`, a
  w niektórych dystrybucjach także `ncurses-dev`, `libsdl-dev`, `curl-dev`
  (nazwy pakietów mogą się nieco różnić w zależności od dystrybucji)
4. Uruchomić `./configure` z opcjami wedle uznania (patrz `./configure
  --help`). Oficjalna binarka była kompilowana z:
  ```
  --target-list=i386-softmmu,x86_64-softmmu --python=$(which python2) --audio-drv-list=alsa,pa
  ```
5. Wykonać `make`
6. Zainstalować wykonując `make install`, lub uruchomić bezpośrednio (binarka
   to `x86_64-softmmu/qemu-system-x86_64`).

Aby zmodyfikowane qemu emulowało urządzenie HardDoom™, należy przekazać mu
opcję `-device harddoom`. Przekazanie tej opcji kilka razy spowoduje emulację
kilku instancji urządzenia.

Aby dodać na żywo (do działającego qemu) urządzenie HardDoom™, należy:

- przejść do trybu monitora w qemu (_Ctrl+Alt+2_ w oknie qemu)
- wpisać `device_add harddoom`
- przejść z powrotem do zwykłego ekranu przez _Ctrl-Alt-1_
- wpisać `echo 1 > /sys/bus/pci/rescan`, aby linux zauważył

Aby udać usunięcie urządzenia

```
echo 1 > /sys/bus/pci/devices/0000:<idurządzenia>/remove
```

## Testy


Do testowania sterownika przygotowaliśmy zmodyfikowaną wersję `prboom-plus`,
który jest uwspółcześnioną wersją silnika gry Doom. Aby go uruchomić, należy:

- zainstalować w obrazie paczki:

  - `libsdl2-dev`
  - `libsdl2-mixer-dev`
  - `libsdl2-image-dev`
  - `libsdl2-net-dev`
  - `xfce4` [albo inne środowisko graficzne]
  - `xserver-xorg`
  - `autoconf`

- pobrać źródła z repozytorium https://github.com/koriakin/prboom-plus
- wybrać branch `doomdev`
- skompilować źródła (bez instalacji, program nie będzie w stanie znaleźć
  swojego pliku z danymi, `prboom-plus.wad`):

  - `./bootstrap`
  - `./configure --prefix=$HOME`
  - `make`
  - `make install`

- pobrać plik z danymi gry.  Można użyć dowolnego z następujących plików:

  - `freedoom1.wad` lub `freedoom2.wad` z projektu Freedoom
    (https://freedoom.github.io/) - klon gry Doom dostępny na wolnej licencji.
  - `doom.wad` lub `doom2.wad` z pełnej wersji oryginalnej gry, jeśli
    zakupiliśmy taką.
  - `doom1.wad` z wersji shareware oryginalnej gry.

- załadować nasz sterownik i upewnić się, że mamy dostęp do `/dev/doom0`
- uruchomić X11, a w nim grę: `$HOME/bin/prboom-plus -iwad <dane_gry.wad>`
- w menu _Options -> General -> Video_ mode wybrać opcję "doomdev" (domyślne
  ustawienie "8bit" wybiera renderowanie programowe w trybie bardzo podobnym
  do naszego urządzenia i można go użyć do porównania wyników).

Aby w grze działał dźwięk, należy przekazać do qemu opcję `-soundhw hda` i
przy kompilacji jądra włączyć stosowny sterownik (_Device Drivers -> Sound card
support -> HD-Audio_).

## Wskazówki

Do plików dla buforów ramek, tekstur, itp. polecamy użyć funkcji
`anon_inode_getfile`. Niestety, tak utworzone pliki nie pozwalają domyślnie
na `lseek`, `pread`, itp - żeby to naprawić, należy ustawić flagi
`FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE` w polu `f_mode`.

Aby dostać strukturę `file` z deskryptora pliku, możemy użyć `fdget` i `fdput`.
Aby sprawdzić, czy przekazana nam struktura jest odpowiedniego typu, wystarczy
porównać jej wskaźnik na strukturę `file_operations` z naszą.

Polecamy zacząć implementację od operacji `FILL_COLOR` i `DRAW_LINE` (wymagają
tylko bufora ramki i pozwalają zobaczyć mapę).  Następnie polecamy
`DRAW_COLUMN` (można na początku pominąć flagi i mapy kolorów) - jest
odpowiedzialna za rysowanie większości grafiki w grze i bez niej niewiele
zobaczymy.

Urządzenie wymaga, by rozmiar tekstury był wielokrotnością 256 bajtów.
W przypadku stworzenia tekstury o rozmiarze niewspieranym bezpośrednio przez
sprzęt, należy wyrównać rozmiar w górę i dopełnić dane od użytkownika
zerami.

Rozmiar tekstury czy bufora ramki rzadko kiedy jest dokładnie wielokrotnością
strony - możemy to wykorzystać, umieszczając tabelę stron w nieużywanej
części ostatniej strony.  Pozwoli to uniknąć osobnej alokacji na (zazwyczaj
bardzo małą) tabelę stron.

Do rozwiązania w niżej punktowanej wersji synchronicznej nie jest konieczne
użycie polecenia `FENCE` i powiązanych rejestrów -- wystarczy samo
`PING_SYNC`.  W rozwiązaniu w pełnej wersji asynchronicznej konieczne będzie
użycie `FENCE` (w połączeniu z rejestrem `FENCE_WAIT` lub poleceniem
`PING_SYNC` do oczekiwania w `read`).

Może się zdarzyć, że nie będziemy w stanie wysłać polecenia ze względu na
brak miejsca w kolejce poleceń (tej wbudowanej w urządzenie bądź naszej
własnej w pamięci wskazywanej przez `CMD_*_PTR`).  Żeby efektywnie
zaimplementować oczekiwanie na wolne miejsce, polecamy:

- wysyłać z jakąś minimalną częstotliwością (np. co _1/8 .. 1/2_ wielkości
  naszego bufora poleceń bądź sprzętowej kolejki) polecenie `PING_ASYNC`
- domyślnie ustawić przerwanie `PONG_ASYNC` na wyłączone
- w razie zauważenia braku miejsca w kolejce:

  - wyzerować przerwanie `PONG_ASYNC` w `INTR`
  - sprawdzić, czy dalej nie ma miejsca w kolejce (zabezpieczenie przed
    wyścigiem) -- jeśli jest, od razu wrócić do wysyłania
  - włączyć przerwanie `PONG_ASYNC` w `INTR_ENABLE`
  - czekać na przerwanie
  - z powrotem wyłączyć przerwanie `PONG_ASYNC` w `INTR_ENABLE`

Aby urządzenie działało wydajnie, należy unikać niepotrzebnego wysyłania
poleceń wymuszających serializację (przede wszystkim między poleceniami
z jednego wywołania `ioctl`):

- `*_PT`, `*_ADDR`: czyszczą pamięć podręczną i blokują paczkowanie
  sąsiednich kolumn przez mikrokod (w szczególności, jeśli dwie sąsiednie
  kolumny mają taki sam `colormap_idx`, nie należy wysyłać między nimi
  polecenia `COLORMAP_ADDR`).
- `SURF_DIMS`, `TEXTURE_DIMS`, `DRAW_PARAMS`, `FENCE`, `PING_SYNC`:
  blokują paczkowanie kolumn (ale są w zasadzie darmowe pomiędzy paczkami).
- `INTERLOCK`: blokuje równoległe przetwarzanie poleceń `COPY_RECT` (jeśli
  chcemy czytać z bufora, do którego ostatnio rysowaliśmy przed wysłaniem
  ostatniego polecenia `INTERLOCK`, nie ma potrzeby wysyłać kolejnego - w
  szczególności, w przypadku serii wywołań `COPY_RECT` między różnymi buforami
  ramki, nie należy wysyłać `INTERLOCK` między wywołaniami).

Nie trzeba się za to przejmować nadmiarowymi poleceniami `FILL_COLOR`, `XY_*`,
`*START`, `*STEP`, `PING_ASYNC` - przetworzenie ich przez urządzenie nie zajmie
więcej niż sprawdzenie nadmiarowości przez sterownik.

Jeżeli chcemy tymczasowo (do testów) pozmieniać coś w grze (np. wykomentować
operacje, których jeszcze nie wspieramy), kod odpowiedzialny za obsługę
urządzenia możemy znaleźć w `src/i_doomdev.c`.

Jeżeli popsujemy konfigurację `prboom-plus` tak, że przestanie się uruchamiać w
stopniu wystarczającym do zmiany ustawień, możemy znaleźć jego plik
konfiguracyjny w `$HOME/.prboom-plus/prboom-plus.cfg`. Usunięcie go spowoduje
przywrócenie ustawień domyślnych.


Aby zobaczyć różne operacje w akcji:

- `DRAW_COLUMN` bez żadnych flag: używane do rysowania grafik interfejsu (menu,
  itp).
- `DRAW_COLUMN` z `COLORMAP`: używane do rysowania wszystkich ścian i
  większości obiektów.
- `DRAW_COLUMN` z `TRANSLATE`: używane do rysowania nowego HUD (dostępnego pod
  przyciskiem F5 -- być może po naciśnięciu kilka razy).  Jeśli zmiana palety
  działa, część cyfr powinna nie być czerwona.
- `DRAW_COLUMN` z `FUZZ`: wpisujemy kod `idbeholdi`, żeby uczynić się
  niewidzialnym (po prostu naciskając kolejne litery w trakcie rozgrywki).
- `DRAW_SPAN`: używane do rysowania podłogi / sufitu.
- `DRAW_LINE` i `FILL_COLOR`: używane do rysowania mapy (przycisk Tab).
- `COPY_RECT`: używane do efektu przejścia między stanami gry (choćby
  rozpoczęcie nowej gry czy ukończenie poziomu).
- `DRAW_BACKGROUND`: zmniejszamy rozmiar ekranu z pełnego (naciskając przycisk
  `-` kilka razy) - ramka ekranu powinna być wypełniona.
