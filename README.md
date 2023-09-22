# tp-2023-2c-Sisop-Five

gcc -I"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/include/" -L"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" -o memoria main.c /home/utnso/tp-2023-2c-Sisop-Five/memoria/src/conexion.c -lmappaLib -lcommons
LD_LIBRARY_PATH="/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" ./memoria

gcc -I"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/include/" -L"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" -o filesystem main.c -lmappaLib -lcommons
LD_LIBRARY_PATH="/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" ./filesystem

gcc -I"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/include/" -L"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" -o cpu main.c -lmappaLib -lcommons
LD_LIBRARY_PATH="/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" ./cpu

gcc -I"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/include" -L"/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" -o kernel main.c -lmappaLib -lcommons -lreadline
LD_LIBRARY_PATH="/home/utnso/tp-2023-2c-Sisop-Five/mappaLib/Debug" ./kernel
