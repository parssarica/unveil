gcc -Wall -Wextra -pedantic -o ptracer queuesage.c ptracer.c -lcjson `pkg-config --cflags --libs glib-2.0`
echo "------------------------------------------------------------------"
gcc -o example example.c unveilptrace.c queuesage.c -lcjson
