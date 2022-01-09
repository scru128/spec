# SCRU128: Sortable, Clock and Random number-based Unique identifier

SCRU128 ID is yet another attempt to supersede [UUID] in the use cases that need
decentralized, globally unique time-ordered identifiers. SCRU128 is inspired by
[ULID] and [KSUID] and has the following features:

- 128-bit unsigned integer type
- Sortable by generation time (as integer and as text)
- 26-digit case-insensitive portable textual representation
- 44-bit biased millisecond timestamp that ensures remaining life of 550 years
- Up to 268 million time-ordered but unpredictable unique IDs per millisecond
- 84-bit [_layered_ randomness](#layered-randomness) for collision resistance

Examples in the 26-digit canonical textual representation:

```
00QBTG0FERFTCCNFISUBO489DI
00QBTG0FERFTCCPFISU9NEF4CJ
00QBTG0FERFTCCRFISU97R5OTB
00QBTG0FERFTCCTFISUA4B4DFK
00QBTG0FF6ULUKDFISU93H7KA9
00QBTG0FF6ULUKFFISU9OC8U2F
00QBTG0FF6ULUKHFISUAOVFSF4
00QBTG0FF6ULUKJFISU94NVHK2
```

[uuid]: https://en.wikipedia.org/wiki/Universally_unique_identifier
[ulid]: https://github.com/ulid/spec
[ksuid]: https://github.com/segmentio/ksuid

## Implementations

- [C/C++](https://github.com/scru128/c)
- [Go](https://github.com/scru128/go-scru128)
- [Java (with Kotlin and Android compatibility)](https://github.com/scru128/java)
- [JavaScript](https://github.com/scru128/javascript)
- [Python (and command-line tools)](https://github.com/scru128/python)
- [Rust](https://github.com/scru128/rust)
- [Swift](https://github.com/scru128/swift-scru128)

For those interested in implementing SCRU128: [SCRU128 Generator Tester]

[scru128 generator tester]: https://github.com/scru128/gen_test

## Specification v1.0.2

A SCRU128 ID is a 128-bit unsigned integer consisting of four terms:

```
timestamp * 2^84 + counter * 2^56 + per_sec_random * 2^32 + per_gen_random
```

Where:

- `timestamp` is a 44-bit unix time in milliseconds biased by 50 years (i.e.
  milliseconds elapsed since 2020-01-01 00:00:00+00:00, ignoring leap seconds;
  or, the unix time in milliseconds minus `1577836800000`).
- `counter` is a 28-bit counter incremented by one for each new ID generated
  within the same `timestamp` (reset to a 28-bit random number at initial
  generation and whenever `timestamp` changes).
- `per_sec_random` is a 24-bit random number renewed once per second (i.e. a
  random number shared by all the IDs generated within the same second).
- `per_gen_random` is a 32-bit random number renewed every time a new ID is
  generated.

This is essentially equivalent to allocating four unsigned integer fields to a
128-bit space as follows in a big-endian system, and thus it is easily
implemented with binary operations.

| Bit numbers  | Field name     | Size    | Data type        |
| ------------ | -------------- | ------- | ---------------- |
| Msb 0 - 43   | timestamp      | 44 bits | Unsigned integer |
| Msb 44 - 71  | counter        | 28 bits | Unsigned integer |
| Msb 72 - 95  | per_sec_random | 24 bits | Unsigned integer |
| Msb 96 - 127 | per_gen_random | 32 bits | Unsigned integer |

### Textual representation

A SCRU128 ID is encoded in a string as a 128-bit unsigned integer denoted in the
radix of 32 using the digits of `0-9A-V` (`0123456789ABCDEFGHIJKLMNOPQRSTUV`),
with leading zeros added to form a 26-digit canonical representation. The
following pseudo equation illustrates the encoding algorithm:

```
1095473244246772103403544935821223346
    =  0  * 32^25 +  0  * 32^24 + 26  * 32^23 + ... +  9  * 32^2 + 13  * 32^1 + 18
    = '0' * 32^25 + '0' * 32^24 + 'Q' * 32^23 + ... + '9' * 32^2 + 'D' * 32^1 + 'I'
    = "00QBTG0FERFTCCNFISUBO489DI"
```

Although a 26-digit base-32 numeral can encode 130-bit information, any numeral
greater than `7VVVVVVVVVVVVVVVVVVVVVVVVV` (`2^128 - 1`, the largest 128-bit
unsigned integer) is not a valid SCRU128 ID.

For the sake of uniformity, an encoder should use uppercase letters in encoding
IDs. A decoder, on the other hand, must always ignore cases when interpreting or
lexicographically sorting encoded IDs.

Converters for this simple base 32 notation are widely available in many
languages; even if not, it is easily implemented with bitwise operations by
translating each 5-bit group into one digit of `0-9A-V`, from the least
significant digit to the most. Note that this encoding is different from some
binary-to-text encodings referred to as _base32_ or _base32hex_ (e.g. [RFC
4648]), which read and translate 5-bit groups from the most significant one to
the least.

[rfc 4648]: https://www.ietf.org/rfc/rfc4648.txt

### Other considerations

#### Quality of random numbers

A generator should employ a cryptographically strong random or pseudorandom
number generator in order to generate unpredictable IDs.

#### Reserved IDs

The IDs with `timestamp` being set at zero or `2^44 - 1` are reserved for system
uses and must not be used or assigned as an ID of anything.

#### Layered randomness

SCRU128 utilizes monotonic `counter` to guarantee the uniqueness of IDs with the
same `timestamp`; however, this mechanism does not ensure the uniqueness of IDs
generated by multiple generators that do not share a `counter` state. SCRU128
relies on random numbers to avoid collisions in such a situation.

For a given length of random bits, the greater the number of random numbers
generated, the higher the probability of collision. Therefore, SCRU128 gives
some random bits a longer life to reduce the number of random number generation
per a unit of time. As a result, even if each of multiple generators generates a
million IDs at the same millisecond, no collision will occur as long as the
random numbers generated only once per second (`per_sec_random`) differ.

Note that, however, the `per_sec_random` field must be refreshed every second to
prevent potential attackers from using this field as a generator's fingerprint.
The 32-bit `per_gen_random` field must also be reset to a new random number
whenever an ID is generated to make sure the adjacent IDs generated within the
same `timestamp` are not predictable.

### License

This work is licensed under a [Creative Commons Attribution 4.0 International (CC BY 4.0) License](http://creativecommons.org/licenses/by/4.0/).
