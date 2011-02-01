#!/usr/sbin/dtrace -s

oflux$1:::aha-exception-begin-throw
{
    printf("\n\nGot AHA Exception with stack: \n");
    ustack(20, 100);
}
