# printf self-test cases

These cases are kept separate from the production Banner because the grader compares serial output byte for byte.

| Call | Expected output |
|---|---|
| `printf("%d", 0)` | `0` |
| `printf("%d", -1)` | `-1` |
| `printf("%d", INT_MIN)` | `-2147483648` |
| `printf("%d", INT_MAX)` | `2147483647` |
| `printf("%x", 0)` | `0x0` |
| `printf("%x", 0x48)` | `0x48` |
| `printf("%s", "")` | empty output |
| `printf("%%")` | `%` |

The signed conversion avoids negating `INT_MIN` directly, and hexadecimal conversion emits lowercase digits without leading zeroes.
