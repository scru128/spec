# SCRU128: Sortable, Clock and Random number-based Unique identifier

SCRU128 ID is yet another attempt to supersede [UUID] for the users who need
decentralized, globally unique time-ordered identifiers. SCRU128 is inspired by
[ULID] and [KSUID] and has the following features:

- 128-bit unsigned integer type
- Sortable by generation time (as integer and as text)
- 25-digit case-insensitive textual representation (Base36)
- 48-bit millisecond Unix timestamp that ensures useful life until year 10889
- Up to 281 trillion time-ordered but unpredictable unique IDs per millisecond
- 80-bit [three-layer randomness](#design-notes-three-layer-randomness) for
  global uniqueness

Examples in the 25-digit canonical textual representation:

```
0372hg16csmsm50l8dikcvukc
0372hg16csmsm50l8djl6xi25
0372hg16csmsm50l8dmgepzz1
0372hg16csmsm50l8doir3827
0372hg16cy3nowracls909wcd
0372hg16cy3nowraclvp355ce
0372hg16cy3nowraclxf2ctzh
0372hg16cy3nowraclyunyjke
```

[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier
[ULID]: https://github.com/ulid/spec
[KSUID]: https://github.com/segmentio/ksuid

## Implementations

- [C/C++](https://github.com/scru128/c)
- [Go](https://github.com/scru128/go-scru128)
- [Java (with Kotlin and Android compatibility)](https://github.com/scru128/java)
- [JavaScript](https://github.com/scru128/javascript)
- [Python (and command-line tools)](https://github.com/scru128/python)
- [Rust](https://github.com/scru128/rust)
- [Swift](https://github.com/scru128/swift-scru128)

If you are interested in implementing SCRU128, see also [SCRU128 Generator
Tester](https://github.com/scru128/gen_test).

## Specification v2.0.1

A SCRU128 ID is a 128-bit unsigned integer consisting of four terms:

```
timestamp * 2^80 + counter_hi * 2^56 + counter_lo * 2^32 + entropy
```

Where:

- `timestamp` is a 48-bit Unix timestamp in milliseconds (i.e. milliseconds
  elapsed since 1970-01-01 00:00:00+00:00, ignoring leap seconds).
- `counter_hi` is a 24-bit randomly initialized counter that is incremented by
  one when `counter_lo` reaches its maximum value. `counter_hi` is reset to a
  random number when `timestamp` has moved forward by one second or more since
  the last renewal of `counter_hi`.
  - Note: `counter_hi` effectively works like an entropy component (rather than
    a counter) that is refreshed only once per second.
- `counter_lo` is a 24-bit randomly initialized counter that is incremented by
  one for each new ID generated within the same `timestamp`. `counter_lo` is
  reset to a random number whenever `timestamp` moves forward. When `counter_lo`
  reaches its maximum value, `counter_hi` is incremented and `counter_lo` is
  reset to zero.
- `entropy` is a 32-bit random number renewed for each new ID generated.

This definition is equivalent to allocating four unsigned integer fields to a
128-bit space according to the following layout:

| Bit numbers  | Field name | Size    | Data type        | Byte order |
| ------------ | ---------- | ------- | ---------------- | ---------- |
| Msb 0 - 47   | timestamp  | 48 bits | Unsigned integer | Big-endian |
| Msb 48 - 71  | counter_hi | 24 bits | Unsigned integer | Big-endian |
| Msb 72 - 95  | counter_lo | 24 bits | Unsigned integer | Big-endian |
| Msb 96 - 127 | entropy    | 32 bits | Unsigned integer | Big-endian |

Note that this specification does not specify a canonical bit layout of SCRU128
ID. An implementation may employ any binary form of a 128-bit unsigned integer
to represent a SCRU128 ID.

### Textual representation

A SCRU128 ID is encoded in a string using the _Base36_ encoding. The Base36
denotes a SCRU128 ID as a 128-bit unsigned integer in the radix of 36 using the
digits of `0-9a-z` (`0123456789abcdefghijklmnopqrstuvwxyz`), with leading zeros
added to form a 25-digit canonical representation. The following pseudo equation
illustrates the encoding algorithm:

```
1993501768880490086615869617690763354
    =  0  * 36^24 +  3  * 36^23 +  7  * 36^22 + ... + 27  * 36^2 + 29  * 36^1 + 22
    = '0' * 36^24 + '3' * 36^23 + '7' * 36^22 + ... + 'r' * 36^2 + 't' * 36^1 + 'm'
    = "0372ijojuxuhjsfkeryi2mrtm"
```

Although a 25-digit Base36 numeral can encode more than 128-bit information, any
numeral greater than `f5lxx1zz5pnorynqglhzmsp33` (`2^128 - 1`, the largest
128-bit unsigned integer) is not a valid SCRU128 ID.

For the sake of uniformity, an encoder should use lowercase letters in encoding
IDs. A decoder, on the other hand, must always ignore cases when interpreting or
lexicographically sorting encoded IDs.

The Base36 encoding shown above is available by default in several languages
(e.g. `BigInteger#toString(int radix)` and `BigInteger(String val, int radix)`
constructor in Java). Another easy way to implement it is by using 128-bit or
arbitrary-precision integer division and modulo operations. The following C code
illustrates a naive algorithm based on normal arrays and integers:

```c
const uint8_t id[16] = {1,   127, 239, 57, 194, 100, 27,  165,
                        106, 148, 131, 24, 136, 65,  224, 90};

// convert byte array into digit value array
uint8_t digit_values[25] = {0};
for (int i = 0; i < 16; i++) {
  unsigned int carry = id[i];
  for (int j = 24; j >= 0; j--) {
    carry += digit_values[j] * 256;
    digit_values[j] = carry % 36;
    carry = carry / 36;
  }
}

// convert digit value array into string
static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
char text[26];
for (int i = 0; i < 25; i++) {
  text[i] = digits[digit_values[i]];
}
text[25] = '\0';
puts(text); // 0372ijojuxuhjsfkeryi2mrtm
```

See [the attached reference code] for a comprehensive example and test vectors.

[the attached reference code]: https://github.com/scru128/spec/blob/v2.1.0/base36_128.c

### Special-purpose IDs

The IDs with `timestamp` set at zero or `2^48 - 1` are reserved for special
purposes (e.g. use as dummy, error, or example values) and must not be used or
assigned as an identifier of anything.

### Considerations

#### Quality of random numbers

A generator should employ a cryptographically strong random or pseudorandom
number generator to generate unpredictable IDs.

#### Counter overflow handling

Counter overflow occurs at an extremely low probability when the randomly
initialized `counter_hi` and `counter_lo` do not provide sufficient space for
the IDs generated within a millisecond. The recommended approach to handle
counter overflow is to increment `timestamp` and continue in the following way:

1.  Increment `timestamp` by one.
2.  Reset `counter_hi` to zero.
3.  Reset `counter_lo` to a random number.

This approach is recommended over other options such as the "sleep till next
tick" approach because this technique allows the generation of monotonically
ordered IDs in a non-blocking manner. Raising an error on a counter overflow is
generally not recommended because a counter overflow is not a fault of users of
SCRU128.

This approach results in a greater `timestamp` value than the real-time clock.
Such a gap between `timestamp` and the wall clock should be handled as a small
clock rollback discussed below.

#### Clock rollback handling

A SCRU128 generator relies on a real-time clock to ensure the monotonic order of
generated IDs; therefore, it cannot guarantee monotonicity when the clock moves
back. When a generator detects a clock rollback by comparing the up-to-date
timestamp from the system clock and the one embedded in the last generated ID,
the recommended treatment is:

1.  If the rollback is small enough (e.g. a few seconds), treat the `timestamp`
    of the last generated ID as the up-to-date one, betting that the wall clock
    will catch up soon.
2.  Otherwise, reset `timestamp` to the wall clock and `counter_hi` and
    `counter_lo` to random numbers if the monotonic order of IDs is not
    critically important, or raise an error if it is.

This approach keeps the monotonic order of IDs when a clock rollback is small,
while it otherwise resets the generator and proceeds as if another new generator
were created to minimize the chance of collision.

#### Stateless variant

A generator may fill `counter_hi` and `counter_lo` with random numbers if it
generates IDs infrequently. Such a stateless implementation is acceptable,
though not recommended, because the outcome is not distinguishable from
compliant IDs.

### Design notes: three-layer randomness

SCRU128 utilizes timestamps and counters to ensure the uniqueness of IDs
generated by a single generator, whereas it relies on 80-bit entropy in the use
cases with distributed generators. SCRU128 fills the 80-bit field with a random
number when a new ID is infrequently (less than one ID per second) generated.
For the distributed high-load use cases, SCRU128 assigns different lifetimes to
the three entropy components to improve the collision resistance:

1.  24-bit `counter_hi`: reset to a random number every second
2.  24-bit `counter_lo`: reset to a random number every millisecond
3.  32-bit `entropy`: reset to a random number for every new ID generated

The longer lifetimes of `counter_hi` and `counter_lo` reduce the number of
random numbers consumed and accordingly reduce the probability of at least one
collision because, for a given length of random bits, the less the number of
dice throws, the lower the chance of collision.

In other words, generators are assigned to 24-bit `counter_hi` buckets every
second, and thus they will not collide with each other as long as their buckets
differ, even if each generates a bunch of IDs. 24-bit random numbers usually
collide if millions of instances are generated, but the one-second interval of
`counter_hi` renewals decreases the number of trials drastically. Nevertheless,
`counter_hi` is refreshed every second to prevent potential attackers from
exploiting this field as a generator's fingerprint.

Even within the same bucket, the generators will not collide as long as initial
`counter_lo` values are sufficiently distant from each other. Such a near match
probability, if tried only once a millisecond, is much lower than [the simple
birthday collision probability] calculated over all the IDs generated within a
millisecond. `entropy` provides additional protection in the extremely rare
cases where both `counter_hi` and `counter_lo` collide, but it is primarily
intended to ensure a certain level of unguessability of consecutive IDs
generated by a single generator.

[the simple birthday collision probability]: https://en.wikipedia.org/wiki/Birthday_problem

### License

This work is licensed under a [Creative Commons Attribution 4.0 International
(CC BY 4.0) License].

[Creative Commons Attribution 4.0 International (CC BY 4.0) License]: http://creativecommons.org/licenses/by/4.0/
