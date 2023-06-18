# Changelog

## v2.1.0 - unreleased

- Switched default letter case for generators from uppercase to lowercase

## v2.0.1 - 2022-07-31

- Added bit layout guideline
- Made minor improvements

## v2.0.0 - 2022-05-01

### Changed

- Textual representation: 26-digit Base32 -> 25-digit Base36
- Field structure: { `timestamp`: 44 bits, `counter`: 28 bits, `per_sec_random`:
  24 bits, `per_gen_random`: 32 bits } -> { `timestamp`: 48 bits, `counter_hi`:
  24 bits, `counter_lo`: 24 bits, `entropy`: 32 bits }
- Timestamp epoch: 2020-01-01 00:00:00.000 UTC -> 1970-01-01 00:00:00.000 UTC

### Added

- Counter overflow and clock rollback handling guidance

## v1.0.2 - 2022-01-02

- Applied CC BY 4.0 license

## v1.0.1 - 2021-11-22

- Fixed typo

## v1.0.0 - 2021-10-27

- Initial stable release
