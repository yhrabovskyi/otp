This is a part of the diploma project. It has 2 parts.

First part is the source code for STM32L-DISCOVERY kit. Here is only the code that I've written.
That's why some functions are missed. It implements OTP token. HOTP and TOTP are based on
RFC 4226 and RFC 6238. Features of the implementation:
  1. There are 5 users.
  2. Every user is protected with PIN.
  3. User can generate TOTP or HOTP.
  4. User can setup counter that is used in TOTP or HOTP algorithm.

Second part is the project for Eclipse ADT. It's a simple application for Android 2.2 and higher,
which generates TOTP or HOTP and has the same features as the first part, only without PIN protection.

March, August, 2014.