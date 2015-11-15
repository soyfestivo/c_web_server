# c_web_server
This is not my first webserver but it is my first webserver written in C. It is aimed towards
a unix environment but would also work on a windows machine given some equivalent for `<unistd.h>`

This webserver impliments:
- HTTP 1.0/1.1 GET and POST commands
- Multi-threading
- I/O Multiplexing (in progress)
- data caps per user (based off of IP)
- sessions (similar to PHP's $_SESSION)
- and user logins to restricted areas

What started out as a class project has turned into a lot more and I plan to keep improving it

To use, run the commands
```
make web_server
./web_server 80 WWW2/
```