get:
stmia r0, {r0-r15}
mov r0, #0
bx lr

set:
ldmia r0, {r0-r14}
bx lr

swap:
stmia r0, {r0-r15}
ldmia r1, {r0-r14}
bx lr
