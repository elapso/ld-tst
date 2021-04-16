# Про динамический компоновщик и иже с ним
## Введение
Вторая буква в акрониме ELF представляет слово «linkable» и обозначает, что исполняемый код может располагаться в различных файлах и будет собран в единую работоспособную систему после его размещения в памяти. Отвечает за этот процесс компоновщик времени выполнения ld.so (https://man7.org/linux/man-pages/man8/ld.so.8.html), который также является библиотекой, подключаемой ко всем программам, использующим динамическую линковку. Компоновщик представляет из себя статически собранный ELF-файл:

    ldd  /lib64/ld-linux-x86-64.so.2
    statically linked
    
Для того, чтобы не оказаться в положении приснопамятного барона и не заниматься поиском самого себя, путь до библиотеки ***ld.so*** прописывается компилятором в параметре INTERPR ELF-заголовка. Поэтому в другой терминологии данная библиотека фигурирует под названием интерпретатора ELF-файлов (ELF-interpreter).

    readelf -a ./tst | grep interpreter
    [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2] 
 
После загрузки ELF-заголовка первоначального файла и получения пути к ELF- интерпретатору, выполняется его запуск и начинается решение основной задачи динамического компоновщика – поиск необходимых библиотек и размещение их в памяти. 
## Последовательность поиска 
Первым делом динамический компоновщик получает список тех библиотек, которые необходимы для работы программы. Данная информация так же храниться в ELF-заголовке:
    
    readelf -d foo | grep NEEDED
    0x0000000000000001 (NEEDED)             Shared library: [libtst.so]
    0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
  
У компоновщика есть довольно много вариантов для поиска перечисленных библиотек. Для того, чтобы рассмотреть их на практике, воспользуемся переменной среды ***LD_DEBUG***, которая позволяет наблюдать за процессом поиска прямо в консоли:

    LD_DEBUG=libs ./foo TESTDATA

Для иллюстрации экспериментов используется программа foo, которая для своей работы требует наличие библиотеки сам описной ***libtst.so*** и стандартной ***libc.so.6***. 
Исходный текст и команды сборки представлены в репозитарии.

При включенной отладки мы увидим, что поиск библиотеки выполняется в кэше и в системных каталогах:

       841:     find library=libtst.so [0]; searching
       841:      search cache=/etc/ld.so.cache
       841:      search path=/lib/x86_64-linux-gnu/tls/x86_64/x86_64:/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls:/lib/x86_64-linux-gnu/x86_64/x86_64:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu/tls/x86_64/x86_64:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls:/usr/lib/x86_64-linux-gnu/x86_64/x86_64:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu:/lib/tls/x86_64/x86_64:/lib/tls/x86_64:/lib/tls/x86_64:/lib/tls:/lib/x86_64/x86_64:/lib/x86_64:/lib/x86_64:/lib:/usr/lib/tls/x86_64/x86_64:/usr/lib/tls/x86_64:/usr/lib/tls/x86_64:/usr/lib/tls:/usr/lib/x86_64/x86_64:/usr/lib/x86_64:/usr/lib/x86_64:/usr/lib              (system search path)
       841:       trying file=/lib/x86_64-linux-gnu/tls/x86_64/x86_64/libtst.so
       …
       841:       trying file=/usr/lib/libtst.so
       841:
    ./tst: error while loading shared libraries: libtst.so: cannot open shared object file: No such file or directory

Логично, что библиотека, которую мы только что собрали в указных местах не обнаружена. В кэше ее нет, потому что ее никто еще не загружал, в системные каталоги никто не положил. Список системных каталогов состоит из ***/usr/lib, /lib*** и тех, что перечислены в файле конфигурации ***/etc/ld.so.conf***, включая ***/etc/ld.so.conf/d/*.conf*** (а у меня не так). Странно, что компоновщик не удосужился поискать в текущем каталоге, но ведь его об этом никто и не просил : ).
Сделать это можно различными способами.

1.	При сборке ELF-файла указать, что библиотека будет лежать в том же каталоге:

        gcc -o foo tst.c -L. -l:./libtst.so
     
Обратите внимание, что библиотека указана с относительным путем, но ничто не мешает использовать в этом случае и абсолютный.

2.	Переменной окружения LD_LIBRARY_PATH


         LD_DEBUG=libs LD_LIBRARY_PATH=./ ./tst TESTDATA

         :~/work/tst-cpp$ LD_DEBUG=libs LD_LIBRARY_PATH=./ ./tst
         847:     find library=libtst.so [0]; searching
         847:      search path=./tls/x86_64/x86_64:./tls/x86_64:./tls/x86_64:./tls:./x86_64/x86_64:./x86_64:./x86_64:.            (LD_LIBRARY_PATH)
         847:       trying file=./tls/x86_64/x86_64/libtst.so
         …
         847:       trying file=./libtst.so
         847:     find library=libc.so.6 [0]; searching
         847:      search path=./tls/x86_64/x86_64:./tls/x86_64:./tls/x86_64:./tls:./x86_64/x86_64:./x86_64:./x86_64:.            (LD_LIBRARY_PATH)
         847:       trying file=./tls/x86_64/x86_64/libc.so.6
         …
         847:       trying file=./libc.so.6
         847:      search cache=/etc/ld.so.cache
         847:       trying file=/lib/x86_64-linux-gnu/libc.so.6
         847:     calling init: /lib/x86_64-linux-gnu/libc.so.6
         847:     calling init: ./libtst.so
         847:     initialize program: ./tst
         847:     transferring control: ./tst
         847:
         shared foo
         847:     calling fini: ./tst [0]
         847:     calling fini: ./libtst.so [0]

Путь, переданный через переменную ***LD_LIBRARY_PATH***, имеет большой приоритет и поиск начинается с него. Он используется как префикс, который дополняется заранее заданными подкаталогами. Самописная библиотека найдена, компоновщик переходит к поиску glibc. Возникает обратная ситуация – в локальном каталоге она ожидаемо отсутствует. Тогда на помощь приходит кэш динамического компоновщика, который подсказывает, что ее можно взять в каталоге ***/lib/x86_64-linux-gnu***.

Если исключить использование кэша, удалив файл ***./etc/ld.so.cache***, то поиск будет продолжен в системных каталогах. 

3.	Инструкции RPATH и RUNPATH

Другой путь, это указать каталоги поиска библиотек в самом исполняемом файле. Как уже давно повелось в мире IT, для этих целей могут быть использованы два параметра Dynamic секции ELF файла – ***RPATH*** (старый ) и ***RUNPATH (новый)***.
Разница между ними в приоритете применения (об этом ниже) и в том, что новый может быть не распознан старым динамическим компоновщиком. 

Начнем со старого. Добавим в строку сборки параметры линковщика, которые gcc успешно передаст ld:

    gcc -o foo tst.c -L. -l:libtst.so -Wl,-rpath,./
    
Результатом будет поиск всех необходимых библиотек в первую очередь в текущем каталоге:

    LD_DEBUG=libs ./foo TESTDATA
      1243:     find library=libtst.so [0]; searching
      1243:      search path=./tls/x86_64/x86_64:./tls/x86_64:./tls/x86_64:./tls:./x86_64/x86_64:./x86_64:./x86_64:.            (RUNPATH from file ./tst)
      …
      1243:       trying file=./libtst.so
      1243:     find library=libc.so.6 [0]; searching
      1243:      search path=./tls/x86_64/x86_64:./tls/x86_64:./tls/x86_64:./tls:./x86_64/x86_64:./x86_64:./x86_64:.            (RUNPATH from file ./tst)
      …
      1243:       trying file=./libc.so.6
      1243:      search cache=/etc/ld.so.cache
      1243:      search path=/lib/x86_64-linux-gnu/tls/x86_64/x86_64:/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls:/lib/x86_64-linux-gnu/x86_64/x86_64:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu/tls/x86_64/x86_64:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls:/usr/lib/x86_64-linux-gnu/x86_64/x86_64:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu:/lib/tls/x86_64/x86_64:/lib/tls/x86_64:/lib/tls/x86_64:/lib/tls:/lib/x86_64/x86_64:/lib/x86_64:/lib/x86_64:/lib:/usr/lib/tls/x86_64/x86_64:/usr/lib/tls/x86_64:/usr/lib/tls/x86_64:/usr/lib/tls:/usr/lib/x86_64/x86_64:/usr/lib/x86_64:/usr/lib/x86_64:/usr/lib              (system search path)
      …
      1243:       trying file=/lib/x86_64-linux-gnu/libc.so.6
      1243:     calling init: /lib/x86_64-linux-gnu/libc.so.6
      1243:     calling init: ./libtst.so
      1243:     initialize program: ./tst
      
Тот же вызов при использовании RUNPATH. Компиляция:

     gcc -o tst tst.c -L. -l:libtst.so -Wl,--enable-new-dtags -Wl,-rpath,./
     
Содержимое ELF-секции Dynamic:

     objdump -x tst | grep -A3 Dyna
     Dynamic Section:
         NEEDED               libtst.so
         NEEDED               libc.so.6
         RUNPATH              ./
         
Результат выполнения:

     LD_DEBUG=libs LD_LIBRARY_PATH=/home ./tst TESTDATA
      1516:     find library=libtst.so [0]; searching
      1516:      search path=/home/tls/x86_64/x86_64:/home/tls/x86_64:/home/tls/x86_64:/home/tls:/home/x86_64/x86_64:/home/x86_64:/home/x86_64:/home             (LD_LIBRARY_PATH)
      1516:       trying file=/home/tls/x86_64/x86_64/libtst.so
      ...
      1516:      search path=./tls/x86_64/x86_64:./tls/x86_64:./tls/x86_64:./tls:./x86_64/x86_64:./x86_64:./x86_64:.            (RUNPATH from file ./tst)
      1516:       trying file=./tls/x86_64/x86_64/libtst.so
      ...
      1516:       trying file=./libtst.so
      1516:     find library=libc.so.6 [0]; searching
      1516:      search path=/home              (LD_LIBRARY_PATH)
      1516:       trying file=/home/libc.so.6
      1516:      search path=./tls/x86_64/x86_64:./tls/x86_64:./tls/x86_64:./tls:./x86_64/x86_64:./x86_64:./x86_64:.            (RUNPATH from file ./tst)
      ...
      1516:       trying file=./libc.so.6
      1516:
      
Как видим, порядок применения параметра ***RUNPATH*** отличается от ***RPATH*** – он используется после выполнения поиска по пути, указанному в переменной окружения ***LD_LIBRARY_PATH***.
Одновременное использование ***RUNPATH*** и ***RPATH*** не предусмотрено, компоновщик ld не добавит их вместе при линковке. Если сделать это вручную (спасибо Quarkslab)

    import lief 
    b = lief.parse('./foo')
    b += lief.ELF.DynamicEntryRpath('/home/')
    b += lief.ELF.DynamicEntryRunPath('/root/')
    b.write('./foo-both')

, то параметр ***RPATH*** будет проигнорирован динамическим компоновщиком независимо от порядка размещения параметров в секции Dynamic:

    objdump -x foo-both | grep -A29 Dynamic
    Dynamic Section:
      NEEDED               libtst.so
      NEEDED               libc.so.6
      …
      RPATH                /home/
      RUNPATH              /root/

Выполнение:

    LD_DEBUG=libs ./foo-both
      3085:     find library=libtst.so [0]; searching
      3085:      search path=/home/tls/x86_64/x86_64:/home/tls/x86_64:/home/tls/x86_64:/home/tls:/home/x86_64/x86_64:/home/x86_64:/home/x86_64:/home        (RUNPATH from file ./foo-runp)
      3085:       trying file=/home/tls/x86_64/x86_64/libtst.so
      …
      3085:      search cache=/etc/ld.so.cache
      …
      3085:      search path=/lib/x86_64-linux-gnu/tls/x86_64/x86_64:/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls:/lib/x86_64-linux-gnu/x86_64/x86_64:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu/tls/x86_64/x86_64:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls:/usr/lib/x86_64-linux-gnu/x86_64/x86_64:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu:/lib/tls/x86_64/x86_64:/lib/tls/x86_64:/lib/tls/x86_64:/lib/tls:/lib/x86_64/x86_64:/lib/x86_64:/lib/x86_64:/lib:/usr/lib/tls/x86_64/x86_64:/usr/lib/tls/x86_64:/usr/lib/tls/x86_64:/usr/lib/tls:/usr/lib/x86_64/x86_64:/usr/lib/x86_64:/usr/lib/x86_64:/usr/lib             (system search path)
      3085:       trying file=/lib/x86_64-linux-gnu/tls/x86_64/x86_64/libtst.so
   
     ./foo-both: error while loading shared libraries: libtst.so: cannot open shared object file: No such file or directory

Итого - последовательность поиска разделяемых библиотек динамическим компоновщиком: 
1. RPATH (но только, если нет RUNPATH)
2. LD_LIBRARY_PATH 
3. RUNPATH 
4. system search path

## Особые случаи
### Переменной окружения LD_PRELOAD

Любимый способ компьютерных злодеев и прочих любителей обходных путей. Использование ***LD_PRELOAD*** или эквивалентное изменение файла ***/etc/ld.so.preload*** предписывает динамическому компоновщику использовать перечисленные библиотеки в первую очередь. Но обратит внимание, что компоновщик сначала выполняет поиск библиотек в ранее описанном порядке и только после обнаружения соответствующего разделяемого объекта проверяет, а не стоит ли заменить его на тот, что указан в ***LD_PRELOAD***. Другими словами, если у вас есть библиотека ***lib1.so***, расположенная по нестандартному пути ***/opt/libs/*** и вы хотите загрузить вместо нее ***/home/bob/lib1.so***, то недостаточно указать ***LD_PRELOAD=/home/bob/lib1.so***. Необходимо любым из перечисленных выше способов передать компоновщику информацией о том, где он должен искать ***lib1.so***. Загрузка ***/home/bob/lib1.so*** будет выполнена только обнаружения ***lib1.so***  в любом из проверенных каталогов. Если файл не будет найден, то и подмена выполнена не будет.

Среди прочих, использование данного механизма позволит загрузить нужную версию стандартной библиотеки С++ в случае несоответствия версии библиотеки, использованной при сборке и той, что установлена в ОС в целевой операционной системе. Например:

       Error: /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.26' not found (required by foo-app)

Первым делом выясняем подробности.

Установленная версия библиотеки:

      ls -la /lib/x86_64-linux-gnu/libc.so.6 
      /lib/x86_64-linux-gnu/libc.so.6 -> libc-2.27.so
      ls -la /usr/lib/x86_64-linux-gnu/libstdc++.so.6
      /usr/lib/x86_64-linux-gnu/libstdc++.so.6 -> libstdc++.so.6.0.25
      
Требуемая версии библиотеки:

      objdump -p foo-app
      ...

    требуется из libstdc++.so.6:
    0x056bafd3 0x00 14 CXXABI_1.3
    0x0297f870 0x00 13 GLIBCXX_3.4.20
    0x0297f868 0x00 12 GLIBCXX_3.4.18
    0x02297f89 0x00 11 GLIBCXX_3.4.9
    0x0297f876 0x00 10 GLIBCXX_3.4.26
    0x0bafd179 0x00 09 CXXABI_1.3.9
    0x0297f861 0x00 07 GLIBCXX_3.4.11

Что именно требуется из GLIBCXX_3.4.26:

    objdump -T  foo-app | grep 'GLIBCXX_3\.4\.26'
    0000000000000000      DF *UND*    0000000000000000  GLIBCXX_3.4.26 _ZNSt19_Sp_make_shared_tag5_S_eqERKSt9type_info 

Достаем где-нибудь требуемую версию библиотеки и проверяем, как экспортируется нужный символ:

    objdump -T ./custom-libs/libstdc++.so.6 | grep 
    _ZNSt19_Sp_make_shared_tag5_S_eqERKSt9type_info
    00000000000d0530 g    DF .text    0000000000000031  GLIBCXX_3.4.26 _ZNSt19_Sp_make_shared_tag5_S_eqERKSt9type_info
 
Все в порядке, запуск программы возможен следующим образом:

    LD_PRELOAD=./custom-libs/libstdc++.so.6 ./foo-app

### Литерал $ORIGIN
Конечно, использование ссылки на текущий каталог «./» является не более чем примером и в реальной ситуации не сможет помочь с поиском нужных библиотек. Для этого больше подходит специальный литерал ***$ORIGIN***, который динамический компоновщик преобразует в абсолютный путь до загружаемого ELF-файла.

## GLIBC
Все перечисленные ухищрения будут напрасны в случае замены стандартной библиотеки С от Фонда свободного программного обеспечения. Если задача в том, чтобы использовать версию glibc, отличную от установленной в операционной системе, то недостаточно будет указать динамическому компоновщику, где искать подходящий libc.so. С большой долей вероятности на этапе загрузки возникнет ошибка сегментирования или не будет обнаружен какой-либо экспортируемый символ. Причина в том, что версия стандартной библиотеки должна соответствовать версии динамического компоновщика, выполняющего загрузку. Поэтому менять их нужно совместно. 

Рассмотрим на примере приложения foo, которое вызывает функцию динамически загружаемой библиотеку ***libtst.so***,  требующую gibc версии 2.17.
Для его успешного запуска в операционной системе с более низкой версией стандартной библиотеки нам понадобиться:

•	foo – само приложение;
•	libtst.so – динамически загружаемая библиотека;
•	libc.so.6 – glibc версии не ниже 2.17, в нашем случае 2.28;
•	ld-2.28.so – динамический компоновщик, версия которого строго соответствует версии libc.so.6.

При сборке приложения foo необходимо указать, откуда загружать компоновщик и где искать необходимые библиотеки. Для этого передадим линковщику соответствующие параметры:

     gcc -o foo-rpath-interp tst.c -L. -l:./libtst.so -Wl,-I -Wl,./ld-2.28.so -Wl,-rpath,./
     
подразумевая, что динамический компоновщик располагается в текущем каталоге и поиск необходимых библиотек выполняется там же.

После компиляции ELF-заголовок будет содержать ссылку на нужный динамический компоновщик:

      readelf -a ./foo-rpath-interp | grep interpreter
      [Requesting program interpreter: ./ld-2.28.so]
      
После переноса всего необходимого набора ELF-ов на целевую систему и копирования их в один каталог, приложение будет успешно запускаться, но только в том случае, если текущий каталог будет совпадать с каталогом размещения. Исправить это поведение позволят манипуляциями с относительными и абсолютными путями и литералом ***$ORIGIN***.


  
