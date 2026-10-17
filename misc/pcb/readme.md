# PCB Security

Schematic security, bare-board layout, and the assembled board (the PCBA) fail in different ways. A fuse blown in the chip does nothing for a flash chip sitting in the clear on the SPI bus. A buried trace does nothing if the key is the same on every unit and one of them has already been opened. This note is a catalog of the hardening techniques, the attack each one is aimed at, a product or project that actually ships it, where to buy or how to build it, and the bypass that still exists.

Bypasses are named as an attack class. Voltages, timings, and removal recipes belong in a lab manual, not here.

## Contents

- [Who you are defending against](#who-you-are-defending-against)
- [Secure element, TPM, and on-die HSM](#secure-element-tpm-and-on-die-hsm)
- [Per-device keys](#per-device-keys)
- [Encrypted external memory](#encrypted-external-memory)
- [Lifecycle fuses and anti-rollback](#lifecycle-fuses-and-anti-rollback)
- [Secure boot and signed updates](#secure-boot-and-signed-updates)
- [Authenticated debug](#authenticated-debug)
- [Boot straps and unpopulated debug](#boot-straps-and-unpopulated-debug)
- [ROM bootloaders](#rom-bootloaders)
- [Readout protection](#readout-protection)
- [Tamper sensing and zeroize](#tamper-sensing-and-zeroize)
- [Brownout, watchdog, redundant checks](#brownout-watchdog-redundant-checks)
- [The bus to the secure element](#the-bus-to-the-secure-element)
- [Test points and silkscreen](#test-points-and-silkscreen)
- [A programming domain](#a-programming-domain)
- [Inner layers and guard copper](#inner-layers-and-guard-copper)
- [Blind and buried vias](#blind-and-buried-vias)
- [Shield cans](#shield-cans)
- [X-ray and delayering](#x-ray-and-delayering)
- [Coatings and potting](#coatings-and-potting)
- [Tamper meshes](#tamper-meshes)
- [Labels, glue, and security screws](#labels-glue-and-security-screws)
- [End of line](#end-of-line)
- [Split manufacturing and counterfeits](#split-manufacturing-and-counterfeits)
- [Standards](#standards)
- [Production checklist](#production-checklist)
- [What a startup bench can do](#what-a-startup-bench-can-do)
- [References](#references)



## Who you are defending against

Four benches show up:

- A curious owner has the product, a screwdriver, and a multimeter. They will open the case, photograph the board, and look for a header labeled `SWD` or `UART`.
- A repair shop adds a logic analyzer and a hot-air station. They lift a flash or eMMC, dump it, and solder it back. They solder a header onto empty debug pads.
- A glitch-and-side-channel bench adds a ChipWhisperer-class instrument: crowbar or electromagnetic fault injection, and a power or EM trace correlated against a crypto operation. This is the bench that breaks "the ROM checks the signature" when that check is a single branch.
- A delayering lab has x-ray, grinding, microprobing, and sometimes a focused ion beam. They map inner PCB layers and edit a chip. Almost every control in this note loses to that lab if it is applied alone. The point of the cheaper controls is to make that lab the minimum bar, and to make sure a win against one unit does not give away the whole product line.

| Bench | Arrives with | A design aimed here still loses to |
| --- | --- | --- |
| Curious owner | Screwdriver, multimeter, camera | A labeled header, a boot resistor, a silkscreen that says `SWD` |
| Repair shop | Logic analyzer, hot air | External flash in the clear, empty debug pads, an open ROM loader |
| Glitch and side-channel | ChipWhisperer-class fault and power tools | A single-branch signature check, a key that crosses the PCB, no redundant compare |
| Delayering lab | X-ray, grinding, microprobe, sometimes a focused ion beam | One control used alone: a buried trace, a coating, a blown fuse |

## Secure element, TPM, and on-die HSM

**Attack.** Firmware readout, decapping a general-purpose MCU, or dumping external flash, in order to steal a private key or a seed.

**Technique.** Put the key in a part whose job is to keep it. A secure element (Microchip ATECC608, NXP EdgeLock SE050, Infineon OPTIGA Trust) holds a private key and performs the crypto. A discrete TPM (Infineon OPTIGA TPM SLB9670 and similar) does measured boot and sealed storage for a PC or a larger SoC. An on-die subsystem (Apple Secure Enclave, OpenTitan, the HSM block on an automotive SoC) is the same idea inside the main chip, with its own memory and a smaller attack surface than the application core.

The host asks the element to sign, agree, or unwrap. The host does not receive the private key.

| Choice | Key lives in | What you buy | What crosses the PCB | Pick it when |
| --- | --- | --- | --- | --- |
| Application flash | The MCU image | Nothing extra | The whole secret, once debug or chip-off works | A lab board |
| Secure element | ATECC608, SE050, OPTIGA Trust | A second chip, or a pre-provisioned one | Commands. The private key stays inside | The MCU is a commodity part |
| Discrete TPM | SLB9670-class TPM | A TPM module | The unsealed secret, on LPC or SPI, on the way back to the CPU | A PC or a large SoC that already speaks TPM |
| On-die HSM | Secure Enclave, OpenTitan, an automotive HSM | The fuse map for a chip you already chose | Nothing on the board | The SoC has the block and you can close its lifecycle |

**Who ships it.** Ledger wallets keep the seed in a secure element, and their Donjon group publishes attacks against those designs. YubiKey tokens are built around a secure element. Google's Titan chip and the OpenTitan project are root-of-trust chips with an open design in OpenTitan's case. Apple's Secure Enclave has a UID that never leaves the die, file keys are wrapped to it. Payment terminals and EMV cards are the older version of the same split.

**Buy or build.** ATECC608B, SE050, OPTIGA Trust M, and a TPM 2.0 in SPI or I2C are stocked at Digi-Key and Mouser as loose parts or as a board-mount module. TrustCUSTOM / TrustFLEX and EdgeLock 2GO are the versions that arrive with a certificate already inside, so your factory does not have to be the place that generates the key. On a phone-class or car-class SoC the block is already on the die, you turn it on in the lifecycle fuses rather than buying a second chip.

**Bypass.** The element can be used as an oracle if the protocol lets a stolen board sign or unwrap on behalf of the attacker. Side-channel and fault attacks against the element itself are a research field, Ledger's Donjon and academic secure-element papers exist because the chip is small, not because it is magic. A discrete TPM still hands the unsealed secret to the CPU over LPC or SPI. Pulse Security showed BitLocker keys on that bus: the TPM did its job, and the PCB traces undid the secrecy. Replacing the element with a lookalike works when the host never checks which chip it is talking to.

## Per-device keys

**Attack.** One dumped unit yields a firmware decryption key, an HMAC key, or a signing key that is valid for every unit in the field.

**Technique.** Each board gets its own key, generated inside the secure element or injected from an HSM at the end of the line. The image on the SPI flash is either public (and signed) or encrypted under a key that only that chip can unwrap. A global AES key compiled into the firmware is the failure mode this replaces.

| Scheme | One opened unit reveals | Who must be trusted with the private key | Pick it when |
| --- | --- | --- | --- |
| Global key in the firmware | Every unit | Anyone with the binary | A regular demo |
| Same key burned into every OTP | Every unit | The fixture that injects it | You only needed the key off the flash chip |
| Per-device key injected from an HSM | That unit | The HSM and the fixture | You run the programming station |
| Key generated inside the chip | That unit | The chip | The contract manufacturer must not see secrets |
| Pre-provisioned element (TrustCUSTOM, EdgeLock 2GO) | That unit | The silicon vendor's provisioning | You want certificates to arrive in the reel |

**Who ships it.** Apple NAND is wrapped to the on-die UID, so a chip-off dump of one phone does not decrypt another. YubiKey and Microchip TrustCUSTOM parts are provisioned per device. ESP32 flash-encryption keys are burned into that chip's eFuse. PlayStation and Xbox boot ROMs check per-console secrets for the same reason: a dumped disc key from one console is not the next console's key.

**Buy or build.** Use a provisioned secure element, or a programmer that talks to your HSM (SEGGER Flasher Secure and the vendor's own production tools). The factory log should record the public key or certificate, not the private key. If you must inject a symmetric key, inject it once into OTP and then lock the OTP.

**Bypass.** The injection station is now the crown jewel. A shared wrapping key above the per-device keys collapses the scheme back to one global secret. A debug image that prints the key, left on a fixture laptop, is the usual real-world bypass. Faulting the chip into skipping the "is the key locked?" check has shown up on real parts, see the ESP32 advisories below.

## Encrypted external memory

**Attack.** Chip-off or an interposer on external SPI flash, QSPI, eMMC, NAND, or a DRAM bus. The repair-shop bench does this every week. Readout protection inside the MCU does not cover a memory chip that was never designed to keep a secret.

**Technique.** The SoC encrypts what it stores off-chip. ESP-IDF flash encryption, NXP encrypted XIP (execute-in-place) on i.MX, and Apple's UID-bound NAND are the pattern: the ciphertext sits on the memory bus, the key stays in on-chip OTP or a secure element. Authenticated encryption matters. Confidentiality alone still lets an attacker flip ciphertext and fault the CPU into executing something else.

| Off-chip storage | A chip-off dump yields | Flipping ciphertext | Pick it when |
| --- | --- | --- | --- |
| Plain SPI, eMMC, or NAND | Firmware and keys | Changes what the CPU executes | Nothing secret leaves the package |
| Encrypted, key in OTP (ESP32 flash encryption, i.MX XIP) | Ciphertext | Still possible if the mode has no tag | The part must execute from external flash |
| Authenticated encryption | Ciphertext, and a bad tag should refuse to boot | Needs a fault in the tag check | The image is parsed or executed |
| On-chip flash only | Nothing, until readout protection fails | No off-chip bus | The image fits in the MCU |

**Who ships it.** Espressif documents flash encryption next to secure boot and tells you to turn both on. iPhones and other Apple devices bind NAND to the Secure Enclave UID, which is why a desoldered NAND from a locked phone does not yield the photos. Set-top boxes and game consoles encrypt the off-chip boot flash for the same reason.

**Buy or build.** On ESP32 this is an eFuse feature in the menuconfig, with the key generated on chip. On i.MX it is part of HAB and the encrypted XIP engine, configured before the device is closed. Prefer on-chip key generation to a key you type into a script.

**Bypass.** Chip-off of the flash now yields ciphertext, which is the success case. The remaining attacks move to the SoC: read the key out before the fuse is blown, fault the engine into treating ciphertext as plaintext, or side-channel the AES engine. Espressif's advisories cover fault injection against early ESP32 secure boot and flash encryption (CVE-2019-15894 and AR2020-001 on revision 0/1 silicon) and later side-channel-plus-fault work against flash encryption on some newer parts (the advisory index is the thing to track). An interposer that sits between SoC and flash still works if the SoC can be told to disable encryption, or if the attacker glitches that decision.

## Lifecycle fuses and anti-rollback

**Attack.** A field unit is put back into the development lifecycle, debug is reopened, or an old vulnerable firmware image is written back over a patched one.

**Technique.** One-time fuses (eFuse, OTP) record a lifecycle: open, secure, closed. TI's Sitara and Jacinto parts call the open development state HS-FS and the customer-locked state HS-SE. STM32 product states and RDP levels are the MCU version. A monotonic counter in those fuses is the anti-rollback check: the boot ROM refuses an image whose version is older than the counter. The counter only works if it lives in OTP. A counter in rewritable flash is a variable the attacker restores from a backup image.

| Control | Can you go back | Old firmware boots | Debug comes back | Pick it when |
| --- | --- | --- | --- | --- |
| Fuses left at the factory default | Yes | Yes | Yes | A development kit |
| Version counter in rewritable flash | Yes, restore the image | Yes | Yes | You have not spent an OTP bit yet |
| Reversible product state or RDP | Often, and a mass erase wipes the device | The version check is a separate fuse | After the erase | You still need a field return and will accept a blank chip |
| Lifecycle fuse closed (HS-SE, RDP 2, ESP32 download disable) | No | Only if you also fused a version | No | Production |
| Monotonic counter in OTP | No | No | This fuse does not close debug | Updates are a real channel on a closed device |

**Who ships it.** Phones (Android Verified Boot rollback indexes), game consoles, ESP32 eFuses, and TI HS-SE devices. NIST SP 800-193 calls this out as part of firmware resiliency: protect the image, detect a bad one, and recover without going backwards.

**Buy or build.** The fuses are already in the MCU you bought. The work is procedural. Blow them at the end of the line, after the image is verified, and brick a golden sample on purpose so you know the lock holds. Keep a written map of which fuse does what, because the datasheet names are easy to confuse and some of them are irreversible.

**Bypass.** Shipping units with the fuses still open is the common one, the feature exists and the fixture never ran the last script. Fault injection against the comparison ("is version new enough?") is the research one. A counter that saturates, or a boot path that does not consult the counter (a recovery UART, a second core), skips it entirely.

## Secure boot and signed updates

**Attack.** A modified firmware image, written through an update channel, a leftover bootloader, or a chip-off rewrite of external flash.

**Technique.** The boot ROM, or a small bootloader whose hash is in ROM or OTP, checks a signature before it jumps. The public key is in OTP or ROM. A public key that lives in the same flash as the image can be replaced along with the image. The update path uses the same key, rejects rollback, and has a recovery image that is also signed. Android Verified Boot, Arm PSA, and NXP HAB (AN4581) are the documented versions of this chain.

| Check | Where the trust anchor sits | A rewritten flash image | Pick it when |
| --- | --- | --- | --- |
| None | Nowhere | Boots | Bring-up |
| CRC or hash beside the image | In the same flash | The attacker recomputes it | You want to catch accidents, not attackers |
| Signature, public key in that same flash | In the same flash | The attacker swaps the key and the image together | You have not fused anything yet |
| Signature, hash of the key in OTP or ROM | In silicon | Rejected | Production, including a recovery image |
| Vendor ROM (AVB, HAB, ESP-IDF Secure Boot V2) | ROM plus OTP | Rejected | The chip already has a ROM. Use it |

**Who ships it.** Android devices, i.MX and STM32 secure-boot application notes, ESP-IDF Secure Boot V2, consoles, and cars under ISO/SAE 21434 process requirements. Trezor publishes the firmware and the signing story, the threat they accept is a physical attacker, and the threat they refuse is a silent malicious update.

**Buy or build.** Use the vendor's ROM. Do not write your own signature check in a first-stage bootloader if the ROM already has one. Store the hash of the trusted key in OTP. Sign on a machine that is not the build server everyone SSH into. Test that a bit-flipped image does not boot, and that an image signed with a development key does not boot on a production unit.

**Bypass.** A single-branch "signature ok?" check falls to a fault: the ROM takes the success path without the signature being valid. That is the content of the ESP32 secure-boot advisories. Parser bugs in the bootloader (unchecked length, a confused header) have a long history and do not need a glitch. A second boot path that the signature check does not cover, UART download, USB DFU, a B-core, a recovery pin, is the path that gets used.

## Authenticated debug

**Attack.** JTAG, SWD, or a manufacturer debug mailbox left open, or "closed" with a password that shipped in the manual.

**Technique.** The debug port stays electrically present and refuses commands until the chip checks a certificate or a challenge-response. Arm's ADAC (Authenticated Debug Access Control) is the open spec. STM32 debug authentication is the same idea on those MCUs: a debugger can come back on a returned unit, and a stranger with an SWD probe cannot. This replaces both "leave JTAG on" and "blow the fuse and never debug a field return again."

| Policy | A stranger with a probe | A field return | Pick it when |
| --- | --- | --- | --- |
| Port left open | Reads the chip | You debug it | Bring-up |
| Password shipped in the manual or the firmware | Reads it once the password leaks | You debug it | Almost never. The password becomes public |
| Fuse blown, port gone | Gets nothing | You get nothing | You will never open a returned unit |
| Certificate or ADAC | Gets nothing | You debug that serial, with a cert from your HSM | You need both of the rows above |

**Who ships it.** Phone SoCs and server boards (debug authentication is how vendors RMA a locked part). STM32 on the newer families documents it next to RDP. Automotive HSMs expose authenticated debug so a dealer tester can unlock a specific ECU with a certificate from the OEM.

**Buy or build.** Turn the feature on while the lifecycle is still open. The certificates are issued by your HSM, per device or per batch, with an expiry. A lab cert must not be in the production OTP.

**Bypass.** A leaked or never-expiring lab certificate. A fault on the "certificate valid?" branch, same shape as the secure-boot fault. Debug pins that are also reachable through a second, unauthenticated mailbox the ROM still honors.

## Boot straps and unpopulated debug

**Attack.** The owner or the repair shop finds `BOOT` and `SWD` pins, moves a strap, and enters the ROM.

**Technique.** On a production board the boot-mode pin is strapped to the application path by the copper, not by a resistor a person can lift. Debug nets are absent from the production PCB. A do-not-populate header is only a speed bump: the pads are an invitation to solder a connector. Fixture programming belongs on a bed-of-nails pad that is not in the customer BOM, ideally under a part or on a snap-off rail that is cut before ship.

| Layout | Curious owner | Repair shop with an iron | Pick it when |
| --- | --- | --- | --- |
| Resistor strap to a pin header | Lifts the resistor | Same, faster | A development board |
| Do-not-populate header | Solders a connector onto the pads | Same | You left the footprint "just in case" |
| Copper strap, no pads | No pin to move | Reworks a via or a chip pin | Production |
| Snap-off programming rail | The rail is in the scrap | The rail is in the scrap | High volume, programmed in panel |
| Net not routed at all | Nothing to find | Nothing to find | The ROM fuse is blown as well |

**Who ships it.** Production phones and consoles have no debug header. Development kits for the same SoC are covered in them. The difference is the schematic, not the chip.

**Buy or build.** Two BOMs, or one BOM with a build variant. Review the schematic for `SWDIO`, `SWCLK`, `TCK`, `BOOT0`, `BOOT_MODE`, `UART0_TX` going to a connector. Tie boot straps with copper. After boot, firmware can reuse former debug pins as GPIO so a probe attached later sees a normal pin, not a debug port. That reuse is worthless if the ROM already spoke debug before your code ran, so the fuse still has to disable the ROM's debug.

**Bypass.** Lifting a strap resistor and tying the pin the other way, when you used a resistor. Soldering to the unpopulated pads, when you left the pads. A bed-of-nails footprint that is still reachable through a vent or under a label.

## ROM bootloaders

**Attack.** The chip's mask ROM offers UART, USB, or SPI download. It is there for the factory and for blank parts. It is also there for anyone who can hit the boot pin.

**Technique.** Disable that ROM path with a fuse once the application is installed. ESP32 can permanently disable UART download mode in eFuse, Espressif's own fault-injection advisory treats that disable as part of the fix on later silicon. STM32 system bootloader, NXP ROM loaders, and Nordic ROM recovery are the same door on other parts.

| What you close | How | What is still open |
| --- | --- | --- |
| Nothing | Ship the dev config | UART, USB, and the boot pin |
| The boot pin only | A resistor or a copper strap | The ROM, if they move the pin or the fuse is still open |
| UART download | One eFuse | USB, SPI, or a second core's loader |
| Every ROM download path | The fuse map, checked on a sample | A fault on the fuse read, which is the early ESP32 and nRF52 class |

**Who ships it.** Any vendor secure-boot guide that is worth reading tells you to close the ROM downloader. Field products that skip this step show up in teardown writeups with a live UART prompt.

**Buy or build.** One line in the production script, after the image verifies and before the unit leaves the fixture. Confirm on a sample that the boot pin no longer opens a loader.

**Bypass.** The fuse was never blown. A second loader remains (USB when you only closed UART). A fault during the ROM's "is download allowed?" read of the fuse, which is exactly the class of bug in the early ESP32 and nRF52 advisories.

## Readout protection

**Attack.** An SWD or JTAG probe reads internal flash, including keys that were never supposed to leave.

**Technique.** A lock that makes the debug port unable to read flash. STM32 readout protection (RDP) has an open level, a level where debug and flash are disconnected, and a level that permanently disables debug. Nordic APPROTECT is the nRF version. AVR lock bits are the older, weaker version. The lock has to cover the memory that holds the secret. Lock bits on an external SPI flash do not encrypt that flash.

| Lock | Debug can read flash | You can reopen it | Public hole to price in |
| --- | --- | --- | --- |
| STM32 RDP 0, or lock bits unset | Yes | | The product is open |
| STM32 RDP 1 | Flash drops off after the debugger attaches | Usually, with a mass erase | STM32F0 leaks at this level (WOOT 2017) |
| STM32 RDP 2 | No | No | Fault, decap, or another boot path |
| nRF APPROTECT, older silicon | Intended to be no | With a full erase | Fault during the boot check (CVE-2020-27211) |
| nRF APPROTECT, newer silicon plus the firmware lock | No, if both steps ran | With a full erase | Firmware that never finishes the second lock |
| AVR lock bits | Weakly | Sometimes | A long public literature |
| "Lock" bits on an external SPI or eMMC | The chip-off dump is plaintext | | This is the wrong memory |

**Who ships it.** STM32, nRF52, and almost every MCU datasheet. STM documents the levels in AN5156.

**Buy or build.** Set the highest level you can tolerate for field returns. If you need field debug, use authenticated debug instead of the reversible middle level. Verify by attempting a readback on the fixture and failing the unit if the read succeeds.

**Bypass.** The middle level is where products get burned. On STM32F0, Obermaier and Tatschner (USENIX WOOT 2017) showed that RDP level 1 still leaks: SRAM can be stepped, the option bytes can be downgraded, and a debug race reads flash a word at a time. Their recommendation was the permanent level. Nordic's IN-133 / CVE-2020-27211 is a fault against the boot-time APPROTECT check on older nRF52 silicon, newer revisions require a hardware lock and a firmware lock, and a part whose firmware never finishes that second lock can look closed while it is open. Permanent lock raises the bar to a fault, a decap, or a second boot path. It does not protect an external memory chip.

## Tamper sensing and zeroize

**Attack.** An opened case, a drilled hole, a lifted flash, or a probe on a key bus, done slowly enough that the attacker can read secrets before the device notices.

**Technique.** Sensors watch the enclosure and the die: case-open switches, a light sensor, temperature, voltage, clock, and a mesh (next section). On a trip, the chip erases keys. The response has to happen without the attacker's cooperation, and it has to work while the product is "off," which means a small battery or a zeroizable key store that loses its contents when that rail dies. STM32 and i.MX tamper pins, and monitor ICs such as the Analog Devices DS3645, exist to do this on a board that is not a payment terminal.

| Sensor | It catches | It misses | Needs power while "off" | Pick it when |
| --- | --- | --- | --- | --- |
| None | | An opened case | | Secrets are not stored in the box |
| Case switch | A lid that was opened clumsily | Tape holding the switch down | The monitor rail | You want a trip log, and you have a second mechanism |
| Light | The case opened in a lit room | A dark bench, or tape over the sensor | The monitor rail | It is an extra, never the only sensor |
| Temperature, voltage, clock | A fault outside the window | A glitch inside the window | A coin cell, if it must work unplugged | You are aiming at FIPS physical level 4 |
| Mesh, next section | A drill or a mill that breaks a trace | A gap, or a monitor with no power | A coin cell | PCI PTS or FIPS level 3 |

**Who ships it.** PCI PTS payment terminals (the requirement family used for Verifone, Ingenico, PAX, and others) require active detection and immediate erasure, and they require that one failed mechanism is not enough. FIPS 140-3 physical level 3 is the same idea for cryptographic modules: detect and zeroize. Level 4 adds environmental faults (voltage, temperature) to the list of things that must cause a response.

**Buy or build.** Route the tamper switch and the mesh to the MCU tamper pins or to a DS3645-class monitor, powered by the coin cell that also backs the key RAM. Log the trip. Decide in advance that a tripped unit is dead and must be re-provisioned, because a "press OK to continue" path will be the bypass.

**Bypass.** A single switch, held down with tape while the case is open. A light sensor, avoided by working in the dark. A coin cell removed in the same gesture as the sensor wire, so the monitor never runs. Cold-boot against SRAM: Halderman and others showed that keys in ordinary RAM survive power loss for seconds to minutes, longer if the RAM is chilled, which races a slow zeroize. A sensor whose threshold is wider than the fault you care about (a short voltage glitch inside the brownout window). PCI's answer is at least two independent mechanisms and a mesh, not a better switch.

## Brownout, watchdog, redundant checks

**Attack.** A short glitch on the supply or the clock skips an instruction, flips a branch, or corrupts a register at the moment of a security decision.

**Technique.** A brownout detector resets the chip when the rail sags. A watchdog resets a chip that has stopped making progress. Redundant checks mean the same decision is made twice, in two different ways, so one skipped branch is not enough. Random delays make the interesting cycle harder to hit. These are chip and firmware features, and they belong in the schematic review because the brownout threshold is a pin-strap or a register someone has to set, and the supply network has to be tight enough that a distant glitch actually reaches the detector.

| Control | Raises the cost of | Still loses to | What it costs |
| --- | --- | --- | --- |
| Brownout, threshold near the minimum spec | A supply sag long enough to see | A short glitch inside the window | An on-chip bit |
| Watchdog fed by the real loop | A chip that hangs | A glitch that still pets it, or a reset back into the ROM | An on-chip bit |
| The same check written two ways | One skipped branch | A compiler that merged them, or two faults | Firmware discipline |
| A random delay around the check | A fixed-cycle glitch | A setup that searches | A few lines |
| All four | A ChipWhisperer afternoon | A delayering lab, and a mesh is still a different problem | Mostly free |

**Who ships it.** Secure-boot ROMs that survived public fault-injection reviews (later ESP32 silicon, payment-terminal MCUs) combine a detector with redundant comparisons. ChipWhisperer targets exist specifically so you can see a design that does not do this fail.

**Buy or build.** Enable the on-chip brownout at a threshold close to the minimum spec, not at the bottom of the range. Feed the watchdog from the real control loop. For the one or two decisions that matter (signature ok, debug allowed, version new enough), compare twice and refuse to proceed if the two results differ.

**Bypass.** A glitch shorter than the detector, or a glitch that stays inside the window. A watchdog that the attacker pets, or that resets the chip back into the vulnerable ROM. A redundant check that the compiler merged back into one check. These controls raise the cost of fault injection. They are not a tamper mesh.

## The bus to the secure element

**Attack.** A logic analyzer on the I2C or SPI between the MCU and the secure element or TPM.

**Technique.** Treat that bus as visible. Commands, certificates, and unwrapped session keys will show up on it. The private key should not. Design the protocol so that a transcript of the bus does not yield a long-term secret, and so that a replaced element fails authentication (the host checks the element's certificate, the element checks the host).

| Arrangement | A clip lead on the bus sees | A swapped chip | Pick it when |
| --- | --- | --- | --- |
| TPM that unseals a key to the CPU | The secret, on LPC or SPI | Can impersonate the TPM if the CPU does not check it | You needed measured boot and accept the traces |
| Secure element that signs internally | Commands and responses, not the private key | Works until the host checks a certificate | The MCU must not hold the key |
| Mutual authentication on that bus | The same transcript | Fails | The element can be lifted and replaced |
| On-die HSM, no board-level bus | Nothing | There is no chip to swap | The SoC already has the block |

**Who ships it.** The Pulse Security TPM sniff is the canonical board-level demonstration: the discrete TPM unseals a BitLocker key onto wires anyone with the case open can clip. ATECC608-class parts avoid putting the private key on the bus and still leave every command visible.

**Buy or build.** Read the element datasheet for "what leaves the chip." Prefer a part that can do the sensitive operation internally. If a secret must cross the PCB, wrap it to a key that already lives in both ends, and do not invent that wrapping.

**Bypass.** Watching the bus is the bypass of "we have a TPM, so the key is safe." Replaying or injecting commands is the bypass of an element that does not authenticate the host. Side-channel on the element is the bypass of a bus that was already clean.

## Test points and silkscreen

**Attack.** A labeled pad. `JTAG`, `TX`, `BOOT`, `KEY`, `TP12` next to a flash pin. The curious owner is done in five minutes.

**Technique.** No test points on secrets, clocks, reset, boot straps, or debug. No silkscreen that names those nets. Reference designators can stay for assembly, or they can be scrubbed on a security-only board, scrubbing slows a beginner and does not slow anyone with the datasheet pinout and a continuity meter. Security through unlabeled pins is a delay, and it is worth doing because it is free.

| Marking | Curious owner | Someone with the datasheet and a meter |
| --- | --- | --- |
| Pads named `JTAG`, `TX`, `BOOT`, `KEY` | Done in minutes | Done |
| Reference designators only | Has to trace the pin | Traces the pinout |
| Silkscreen scrubbed | Slower | Same trace |
| No test point on those nets | No pad to clip | Still the chip pin itself |

**Who ships it.** Payment terminals and HSMs leave the security nets unlabeled. Dev kits label everything, on purpose.

**Buy or build.** A schematic checklist item, and a fab note. Keep the real net names in the internal design files. Ship a board whose silkscreen is the assembly legend, not a map of the attack.

**Bypass.** Pinout from the public datasheet, plus continuity from a known ground. Scrubbing the silkscreen does not survive contact with the datasheet.

## A programming domain

**Attack.** The same pads the fixture uses to program the chip are reachable in the field, so the factory interface becomes the customer's shell.

**Technique.** Programming happens on a fixture rail, a pogo footprint, or a connector that is not populated and not reachable after assembly. Power for programming can be a rail the fixture supplies and the product leaves unpowered. After programming, the script locks the chip, and a later step of the line (depanel, shield, pot) covers the pads.

| Fixture | Present on the shipped unit | How the field reaches it |
| --- | --- | --- |
| Header in the customer BOM | Yes | Plug in |
| Do-not-populate pads | The pads | Solder a header |
| Pogo pads under a shield can | The pads, covered | Take the can off |
| Snap-off coupon, depaneled after lock | No | They would need the scrap from the line |
| Programming rail plus a blown fuse | No electrical path | The fuse has to have been blown |

**Who ships it.** High-volume products program on the panel, lock, then depanel. The customer's unit has no programming connector.

**Buy or build.** Panelize with a snap-off programming coupon, or use a bed of nails on test points that the shield can covers. The last electrical step on the fixture is "readback must fail."

**Bypass.** A coupon that ships still attached. Pogo pads under a sticker. A UART left on the main board "for the factory" that is the same UART the ROM listens to.

## Inner layers and guard copper

**Attack.** A needle or a soldered wire on a surface trace that carries a key, a boot strap, or a debug signal.

**Technique.** Route those nets on an inner layer, between ground planes. Flood the outer layers with ground. A guard trace tied to ground on either side of a sensitive analog net makes a probe that slips more likely to short the net to ground, which is noisy and often resets the part. None of this encrypts the signal. It removes the "touch a visible trace" attack.

| Stackup | A sensitive net can sit inside | Rough cost | Pick it when |
| --- | --- | --- | --- |
| 2 layer | No | Lowest | The board has no secret nets |
| 4 layer, signals on the outside | No. The cheap signal-ground-power-signal stack leaves them exposed | Low | You wanted EMI, and security was not the reason |
| 4 layer, ground on the outside | Yes, until a through-via brings the net out | Low | One bus has to disappear from the surface |
| 6 layers or more, secrets between two grounds | Yes | Higher | A payment or HSM board |
| Guard traces around an analog net | The net is still on whatever layer you chose | A design-rule check | A probe that slips should short to ground |

**Who ships it.** Payment-terminal security boards and HSM modules. Ordinary four-layer IoT boards rarely bother, and their SPI flash traces are on the top layer where a clip lead fits.

**Buy or build.** A four-layer stackup is enough to bury a bus (signal, ground, power, signal is the cheap stackup and it does not bury anything, use ground, signal, signal, ground, or a six-layer stack with the secrets on L3). State the stackup in the fab notes. JLCPCB and PCBWay both publish the layer counts they build.

**Bypass.** A via that brings the inner net to a surface pad, including an unused test pad. Grinding or milling down to the inner layer. An x-ray that maps the route so the attacker knows where to grind. Lifting the BGA the net escapes from and probing the pad underneath.

## Blind and buried vias

**Attack.** Every through-via is a surface feature. An inner-layer net that uses a through-via is visible on the outside of the board as a pad or a stub, and it can be probed there.

**Technique.** A blind via connects an outer layer to an inner layer and does not go through. A buried via connects inner layers and never reaches a surface. The sensitive net can change layers without offering a surface dot.

| Via | Dot on the surface | Fab | Pick it when |
| --- | --- | --- | --- |
| Through | Both sides | Included in a normal stackup | The net is not a secret |
| Blind | One side | HDI, laser or controlled-depth | The net must enter an inner layer and stop |
| Buried | None | HDI | The net never needs a surface |
| Delete the net | | Free | You can keep the signal inside the chip |

**Who ships it.** Phones, for density. Security products, for the probing problem. HDI is a standard fab option, not a special security process.

**Buy or build.** HDI on JLCPCB, PCBWay, and any other fab that publishes a capability table. It costs more and it adds design rules (via span, laser versus mechanical). Use it on the debug, key, and strap nets if those nets exist on the board at all. Removing the net is cheaper than burying it.

**Bypass.** X-ray shows the via anyway. Cross-sectioning the board shows it in copper. Blind and buried vias remove the casual probe. They do not survive a lab that is willing to destroy one board to map it.

## Shield cans

**Attack.** A photograph of the chip, or a quick probe of the pins, or EMI coupling into a crypto core that leaks on the air.

**Technique.** A soldered metal can over the SoC, the radio, or the secure element. Electrically it is an EMI shield, and that also reduces the easy electromagnetic side-channel. Mechanically it is a lid.

| Can | Electromagnetic leak | Taking it off | Keys erased when it comes off |
| --- | --- | --- | --- |
| None | A probe sits on the package | | |
| Two-piece, clip lid | Reduced if the frame is grounded | By hand, for development | No |
| One-piece, soldered | Reduced | Hot air | No |
| Can whose ground is also a tamper net | Reduced | The monitor should fire | Yes, if that net feeds the zeroize circuit |

**Who ships it.** Every Wi-Fi module, many SoCs on phones, and crypto devices that have been through an EM side-channel review.

**Buy or build.** Off-the-shelf cans and clips (a two-piece shield you can remove during development and replace with a soldered one-piece in production). Ground it well or it is a decorative hat.

**Bypass.** The can pops off with a pick or hot air. Under it, the board is the same board. A shield is a delay and an EM attenuator. It is not a tamper mesh, because removing it does not zeroize anything unless you also ran a tamper net through the can's ground.

## X-ray and delayering

**Attack.** You buried the trace, scrubbed the silkscreen, and fitted a can. The attacker has an x-ray, or they are willing to sand the board down a layer at a time.

**Technique.** There is no layout trick that hides copper from x-ray. The technique on this side is inspection of your own boards: x-ray a sample from every lot so that an extra chip, an extra bond wire, or a modchip-style implant is something you might catch. Console modchips (PlayStation, Xbox, and later handhelds) are the historical version of "a second board, spliced onto the boot bus, that patches a check." Andrew Huang's supply-chain talks cover the same shape from the factory side: an untrusted assembler is in a perfect position to add a part.

| Check | It sees | It misses | Use it |
| --- | --- | --- | --- |
| Visual and AOI | Wrong body, missing part, bad solder on the surface | Inner layers, and the die inside a package | Every board |
| X-ray | BGA solder, some extra parts and bond wires | A careful implant inside the laminate | A sample from every lot |
| Decap | Die marking and layout, against a golden part | The board you destroyed | First reel of a new vendor, and any dispute |
| IRIS (infrared, in situ) | Features in some chip-scale packages, chip stays on the board | Packages the wavelength cannot enter | When you cannot spend a part on decap |
| Electrical ID | A part that fails the bus or the fuse behavior | A counterfeit that passes those tests | Incoming inspection |

IRIS and electrical ID are checks you run on a reel, not locks inside the product. IRIS (infrared, in situ) uses the fact that silicon is transparent to near-infrared. A modified microscope camera looks through a chip-scale package and photographs the metal pattern on the die while the part stays soldered. You compare that picture to a known-good die. It shows whether the reel contains the chip you ordered. It does not dump flash or read keys, and it only works on packages that wavelength can enter. Electrical ID is the fixture talking to the chip on its normal bus: a JEDEC ID on SPI flash, a JTAG IDCODE, an I2C whoami register, or a secure-element certificate, compared to the part you bought. A remarked chip often fails that exchange, or fails the next step where debug readback must not succeed. A careful counterfeit answers the ID and only misbehaves on the lock bits.

A sensor on the product can erase keys when an inspection starts. It cannot un-take the picture. X-ray of a board is a photograph of copper. Infrared imaging is a photograph of the die. Zeroize after the exposure leaves those images intact. Sanding the board down layer by layer emits neither x-rays nor infrared, so these sensors never see mechanical delayering. That attack is a mesh problem, in the next section.

| Sensor | Buy it | Trips on | Still succeeds |
| --- | --- | --- | --- |
| Visible and near-IR photodiode, the case-open light sensor on an STM32, i.MX, or DS3645-class monitor | Digi-Key, the monitor you already have | A lamp, including the infrared lamp of an optical inspection, if that light reaches the die or the sensor | Imaging done in the dark, or light that never enters the enclosure. The photo, if they took one first |
| Ionizing-radiation detector: a PIN diode, a RADFET, a small Geiger tube, or a scintillator | Physics suppliers such as Hamamatsu. Not a normal security SKU. Some FIPS 140-3 level 4 modules integrate one | A dose high enough to fault memory | Your own factory x-ray, airport scanners, and a PCB x-ray dose too small to notice but large enough to map copper. The radiograph already saved |
| Temperature rate-of-change on the same tamper monitor | The DS3645-class part | Friction heat from a slow grind, if the sensor is still powered and the rise is inside its window | A grind that stays cool, or a grind that removes the sensor first |
| Tamper mesh | Gore-class, or a flex you will not get certified | A drill or a mill that breaks a trace | A path that misses every trace. Covered with the meshes below |

**Who ships it.** Contract manufacturers already x-ray BGAs to check solder. Security programs sample specifically for extra parts and for the right die inside the package (a remarked counterfeit). Radiation zeroize shows up in high-assurance modules, not in IoT BOMs.

**Buy or build.** Use the CM's x-ray, and look at the images yourself on the first lots. For high-assurance lots, decap a sample and compare the die to a golden part. This is a service the CM or a failure-analysis lab sells. For chip-scale packages, infrared inspection (Huang's IRIS work) can show the die without taking the chip off the board.

**Bypass.** Implants between layers, added at the fab rather than at assembly, are much harder to see and much rarer. A patient delayer still reads a buried net. A radiation or light trip that fires late, or that you disabled so the factory x-ray would pass, leaves the keys in place for the photograph. The control is detection and lot rejection, not secrecy of the copper.

## Coatings and potting

**Attack.** Probing, rework, and casual soldering. Also moisture, which is why the coating was invented, the security effect is a side effect.

**Technique.** Conformal coat (acrylic, silicone, polyurethane) is a thin film. Parylene is a vacuum-deposited film, thinner and harder to pick off, from houses such as Specialty Coating Systems. Glob-top is an epoxy dome on one die or one area of the board. Underfill locks a BGA down so it cannot be lifted cleanly. Potting fills the whole enclosure with epoxy or polyurethane. Each of these slows a person with a soldering iron and a logic analyzer.

| Finish | Rework afterwards | Probing | Notices that it was removed | Pick it when |
| --- | --- | --- | --- | --- |
| Bare board | Easy | Easy | No | Development |
| Acrylic, silicone, or urethane conformal | Still possible | Harder | No | The environment needs a coat, and you take the delay |
| Parylene | Hard | Harder | No | You already pay a coater for reliability |
| Glob-top on one die | That die stays put | That die | No | One chip is the target |
| Underfill under a BGA | The part will not lift cleanly | Pins can remain reachable | No | You want chip-off of that BGA to be messy |
| Enclosure filled with potting | No field rework | Slow | No | Meters and cheap tokens |
| Potting plus a mesh | Slow | The mesh should zeroize | Yes | Payment terminals and HSMs |

**Who ships it.** Utility meters, automotive ECUs, and cheap secure tokens get glob-top or potting. High-reliability boards get parylene for the environment, and the security team gets the side effect.

**Buy or build.** Coat and pot at the CM. Leave a process window: a fully potted unit cannot be reworked, so lock the fuses and test the board before the pot. Parylene requires a coater, not a brush.

**Bypass.** A lab with time removes coating and potting. The board underneath is unchanged, and the keys are still there if you did not zeroize on tamper. Potting without a mesh is a delay. It does not notice that it has been removed.

## Tamper meshes

**Attack.** Drilling, milling, grinding, or dissolving your way to a trace or a die, including through potting.

**Technique.** A fine serpentine of conductive ink or traces on a flex or on the inner surface of the enclosure, wrapped so that a hole breaks a track. The monitor watches resistance or continuity and zeroizes. W. L. Gore's tamper-respondent enclosures are the well-known supplier version: a flexible sensor, often potted in, used in modules that go through government and payment certification. PCI PTS writes the requirement as resistance to drills, lasers, solvents, seams, and vents, with erasure as the response, and with a ban on single mechanisms.

| Mesh | A drill | Works while unplugged | A lab will certify it | Pick it when |
| --- | --- | --- | --- | --- |
| A case switch you call a mesh | Only if it hits the switch | Only with a battery on the monitor | No. PCI wants two mechanisms | You have not bought a mesh yet |
| Serpentine on a flex you lay out yourself | If the bit breaks a trace | You supply the coin cell and the monitor | No | A regular enclosure |
| Gore-class respondent, potted in | The pattern plus the potting is what the certifications measure | The module includes the battery-backed monitor | This is the part those labs are used to | PCI PTS or a government module |

**Who ships it.** Payment terminals and certified HSMs. Not hobby boards, and not most IoT products. The cost and the battery-backed monitor are why.

**Buy or build.** Gore and a handful of specialists sell the sensor as a component you design the enclosure around. A homemade serpentine on a flex is a teaching version and will not pass a certification lab. Pair it with the zeroize circuit above, including the coin cell.

**Bypass.** A gap in the pattern, a connector that passes through the mesh, a seam, a vent, or a single switch that was counted as the second mechanism and is not. Finding a path that does not break a trace is the whole game, which is why the certifications measure attack time and skill rather than claiming the mesh is absolute. Bypass of the monitor (hold its input in the "healthy" state, or kill its power together with the mesh) is the board-level version.

## Labels, glue, and security screws

**Attack.** Opening the product without leaving a mark the next owner would notice. This is a different goal from "opening the product." Tamper evidence does not stop an attacker who owns the device. It stops an attacker who has to give it back.

**Technique.** Tamper-evident labels that split or void when peeled. Ultrasonic welds or glue on the housing seam. Fasteners that need an uncommon driver (tri-wing, pentalobe). Apple uses pentalobe and a lot of glue, and also pairs parts to the board cryptographically, so a swapped home button or sensor fails authentication even after the screw is out. The screw is friction. The pairing is the control.

| Control | Slows the owner | The next person can see it was opened | A swapped part fails | Pick it when |
| --- | --- | --- | --- | --- |
| Tri-wing or pentalobe | Until they buy the bit | No | No | You want a matching screwdriver to be a small tax |
| Glue or an ultrasonic weld | Until heat or a crack | If the seam is destroyed | No | The housing should not be a service item |
| Void label | No | Until they apply a fresh label | No | A seal someone is paid to look at |
| Serialized seal you actually check | No | Yes, if the check happens | No | Returns go through your dock |
| Cryptographic pairing of the part | The part fails authentication | No | Yes | A swapped sensor or button must not work |

**Who ships it.** Apple devices (screws, glue, and paired parts). Payment terminals add seals because PCI wants evidence, and they still rely on the mesh for the actual secret. Parcel and meter seals are the same evidence idea outside electronics.

**Buy or build.** Labels from Brady, Avery, and their competitors. A glue path in the mechanical drawing. Pentalobe and tri-wing bits are commodity, do not count them. If a swapped part must not work, authenticate it in the protocol (a certificate in the part, checked by the SoC).

**Bypass.** The matching screwdriver. A heat gun on the glue. A label replaced with a fresh one if you do not serialize the label and check it. Evidence controls lose to an attacker who keeps the device.

## End of line

**Attack.** A unit ships in the development lifecycle because the lock step was manual and got skipped, or because a debug image was flashed "just for this batch."

**Technique.** The fixture script is the security boundary. Program the image, verify the signature, inject or generate the per-device key, blow the lifecycle fuses, disable the ROM bootloader, attempt a debug readback and fail the unit if it succeeds, then cover or remove the programming pads. Log the public identity. Do this in one script so a tired shift cannot reorder it.

| How the lock is applied | A skipped step | What you can audit |
| --- | --- | --- |
| A paper checklist | Often | A tick mark |
| A script on a shared laptop image | A second script, or a debug build flashed "for this batch" | Whatever was left in the repo |
| Vendor programmer or SEGGER Flasher with a fuse map | A wrong map that locks nothing | The programmer log |
| That programmer, authenticated to an HSM | A laptop that does not have the HSM | Public keys and certificates, per serial |

**Who ships it.** Every product that is actually locked in the field. The ones that show up open on a teardown are the ones whose script ended at "flash the image."

**Buy or build.** SEGGER Flasher and the silicon vendor's production programmer both script fuse maps. Keep the production key material in an HSM the programmer authenticates to, not on a file share.

**Bypass.** A second script, a debug build, a fixture in a contract factory that nobody audited, a fuse map that is wrong by one bit and locks the wrong thing (or locks nothing). Sample the line. A golden "this must not debug" unit per lot is cheap.

## Split manufacturing and counterfeits

**Attack.** The fab or the assembler reads your secrets, inserts a board, or substitutes a remarked chip that does not implement the lock you think you programmed. The fab has your Gerbers. The CM has your image. Either one can add a part.

**Technique.** Split the secrets from the geometry. The fab receives Gerbers with no keys in them (keys should never have been in the Gerbers). The CM receives a signed image and cannot sign a different one. Key generation happens inside the chip or inside your HSM at a station you run, possibly after the board comes back. Sample x-ray and sample decap catch extra parts and remarked dies. Buy from authorized distributors so the reel has a pedigree.

| Split | The fab receives | The assembler receives | You still have to check |
| --- | --- | --- | --- |
| One archive for everyone | Gerbers and keys | Image and keys | Everything |
| Geometry and image split | Gerbers | A signed blob they cannot re-sign | Extra parts, remarked dies |
| Keys born in the chip or the HSM | Gerbers | No private key | That the die is the right die |
| Open design (OpenTitan) | A public design | A public design | That the silicon matches the public design |

**Who ships it.** Andrew Huang's supply-chain work, including the 30C3 microSD talk and the matching writeup, is the public tour of this problem: a "dumb" part can hide a whole controller. Console modchips are what an implant looks like when the attacker does it after sale, a malicious CM does it before sale. OpenTitan's open design is the opposite bet: publish the chip so the implant has to be a different chip, and then check that it is the right chip.

**Buy or build.** Authorized distribution (Arrow, Avnet, Digi-Key, Mouser, and the vendor's own store). A written split of who receives which file. Incoming inspection for a high-risk part: electrical ID, and a decap on the first reel of a new vendor.

**Bypass.** A counterfeit that passes electrical tests and only fails the lock bits. An implant in an inner PCB layer, added at a dishonest fab, which your assembly-line x-ray may not notice. A trusted-looking employee laptop with the HSM credentials. Splitting the files reduces who can steal the key. It does not remove the need to look at the boards.

## Standards

These are the documents that turn the catalog into a requirement someone can be audited against.

| Document | What it asks for | What it will not choose for you |
| --- | --- | --- |
| FIPS 140-3 level 1 | Production-grade hardware | Any physical claim |
| FIPS 140-3 level 2 | Tamper-evident coatings or seals | Zeroize |
| FIPS 140-3 level 3 | Detection, zeroize, identity-based authentication | Environmental fault response |
| FIPS 140-3 level 4 | Level 3, plus a response to voltage and temperature | Your stackup |
| PCI PTS | Active response, more than one mechanism, erasure, resistance to drills, solvents, seams, and vents | A hobby enclosure |
| PSA Certified, SESIP | A rated claim for the chip, including a physical story | The case around the chip |
| NIST SP 800-193 | Protect the firmware, detect a bad image, recover without rolling backwards | Which via to use |
| ISO/SAE 21434 | A process: risk, suppliers, incidents | A part number |
| Android Verified Boot | A signed boot chain and a rollback index | The PCB |

- **FIPS 140-3** physical levels. Level 1 is production-grade hardware with no physical claim. Level 2 adds tamper evidence (coatings, seals). Level 3 adds detection and zeroize, and identity-based authentication. Level 4 adds environmental fault response on top of that. [FIPS 140-3](https://csrc.nist.gov/pubs/fips/140-3/final).
- **PCI PTS** (PIN Transaction Security) for payment terminals. Active tamper response, resistance to drilling and solvents and seams, no single point of failure, immediate erasure of keys. [PCI document library](https://www.pcisecuritystandards.org/document_library).
- **Arm PSA Certified** and **SESIP** (GlobalPlatform). Chip and platform security levels that include a physical-attack story rather than only software. [PSA Certified](https://www.psacertified.org/), [SESIP](https://globalplatform.org/sesip/).
- **NIST SP 800-193**, platform firmware resiliency: protect, detect, recover. Signed images, rollback protection, a recovery path that is itself protected. [SP 800-193](https://csrc.nist.gov/pubs/sp/800/193/final).
- **ISO/SAE 21434** for road vehicles. This one is a process standard (risk, suppliers, incident response). It will not tell you which via to use. It will ask you why you chose it. [ISO/SAE 21434](https://www.iso.org/standard/70918.html).
- **Android Verified Boot** is the concrete phone version of secure boot plus rollback indexes. [Verified Boot](https://source.android.com/docs/security/features/verifiedboot).



## Production checklist

Use this on the schematic, the PCB, and the fixture script before a board is called done.

- Boot straps are copper, and the ROM downloader is fuse-disabled.
- No debug header on the production BOM. Programming pads are on a rail, under a shield, or gone after test.
- The public key hash is in OTP. A development key does not boot a production unit.
- External flash is encrypted, or it holds nothing that matters when it is dumped.
- Per-device keys. No global secret in the image.
- Readout protection or authenticated debug is on, and a fixture readback fails closed.
- Anti-rollback counter is in OTP and is consulted by every boot path.
- Tamper, if the threat model includes a returned or resold unit: at least two mechanisms, battery-backed, zeroize, no "continue anyway" path.
- Inner-layer routing for any net you cannot delete. Blind vias only where a surface via would give the net away.
- Silkscreen does not name debug, boot, or key nets.
- Coat or pot only after the lock step. A mesh if you are in PCI or FIPS 140-3 level 3 territory.
- One fixture script: program, verify, inject, blow fuses, fail if debug still reads, log the identity.
- Sample x-ray and a "this unit must not debug" check per lot.



## What a startup bench can do

A ChipWhisperer-class setup, a logic analyzer, and a hot-air station cover the repair-shop column and the glitch column of this note. That bench will dump an unlocked SPI flash, talk to an open SWD port, watch a TPM bus, and fault a single-branch secure-boot check on a chip that has no redundant compare and no fuse. It will not, on a typical afternoon, defeat a payment-terminal mesh, a FIPS 140-3 level 4 enclosure, or a modern secure element with a closed lifecycle and a key that never crosses the PCB.

The useful test sequence is the cheap failures: open debug, a global key in external flash, RDP left at the reversible level, a boot strap made of a resistor, a TPM whose bus is the whole secret. Each of those falls to the first two benches. The rest of the catalog exists so a design that has fixed those still knows which lab it has not priced in.

| If the unit still has | The bench that wins | The upgrade from the tables above |
| --- | --- | --- |
| A header or a boot resistor | Curious owner | Copper strap, or delete the net, and blow the ROM fuse |
| Plain external flash, or RDP left reversible | Repair shop | Encrypt or stop using off-chip storage, and use the permanent lock or authenticated debug |
| A single signature branch, or a key on the SPI bus | Glitch and side-channel | Vendor ROM, redundant compare, key inside an element or on-die |
| A coating, a buried via, or one blown fuse, alone | Delayering lab | Mesh plus zeroize, per-device keys, and a sample inspection |

## References

**Standards and vendor security models**

- [FIPS 140-3](https://csrc.nist.gov/pubs/fips/140-3/final)
- [NIST SP 800-193, Platform Firmware Resiliency](https://csrc.nist.gov/pubs/sp/800/193/final)
- [PCI Security Standards, document library](https://www.pcisecuritystandards.org/document_library) (PTS POI requirements live here)
- [PSA Certified](https://www.psacertified.org/)
- [GlobalPlatform SESIP](https://globalplatform.org/sesip/)
- [ISO/SAE 21434](https://www.iso.org/standard/70918.html)
- [Android Verified Boot](https://source.android.com/docs/security/features/verifiedboot)
- [Apple Platform Security](https://support.apple.com/guide/security/welcome/web), [Secure Enclave](https://support.apple.com/guide/security/secure-enclave-sec59b0b31ff/web), [hardware security overview](https://support.apple.com/guide/security/hardware-security-overview-secf020d1074/web)
- [OpenTitan](https://opentitan.org/)
- [Arm Authenticated Debug Access Control](https://developer.arm.com/documentation/den0101/latest)

**Parts and how you buy them**

- [Microchip Trust Platform](https://www.microchip.com/en-us/products/security/trust-platform) (ATECC608, TrustFLEX, TrustCUSTOM)
- [NXP EdgeLock SE050](https://www.nxp.com/pages/edgelock-se050-plug-and-trust-secure-element-family-enhanced-iot-security-with-high-flexibility:SE050)
- [NXP EdgeLock 2GO](https://www.nxp.com/products/security-and-authentication/authentication/edgelock-2go:EDGELOCK-2GO)
- [Infineon OPTIGA embedded security](https://www.infineon.com/cms/en/product/security-smart-card-solutions/optiga-embedded-security-solutions/) (Trust and TPM families)
- [Analog Devices DS3645 tamper monitor](https://www.analog.com/en/products/ds3645.html)
- [ESP-IDF flash encryption](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html) and [Secure Boot V2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/secure-boot-v2.html)
- [ST AN5156, introduction to STM32 security](https://www.st.com/resource/en/application_note/an5156-introduction-to-security-for-stm32-mcus-stmicroelectronics.pdf)
- [NXP AN4581, secure boot with HAB](https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/imx-processors/194321/1/AN4581_2018.pdf) (2018 public revision)
- [SEGGER Flasher production programmers](https://www.segger.com/products/production/flasher/models/flasher-pro)
- [JLCPCB PCB capabilities](https://jlcpcb.com/capabilities/pcb-capabilities) (layer count and HDI)
- [Specialty Coating Systems, parylene](https://scscoatings.com/)
- [W. L. Gore tamper-respondent enclosure, US patent 5,858,500](https://patents.google.com/patent/US5858500)

**Products that illustrate a choice**

- [Trezor](https://trezor.io/) and the [open firmware](https://github.com/trezor/trezor-firmware)
- [Ledger Donjon](https://donjon.ledger.com/)
- [YubiKey](https://www.yubico.com/products/)
- [Modchips](https://en.wikipedia.org/wiki/Modchip), as the history of PCBA implants on consoles

**Attacks and why the control failed**

- Obermaier and Tatschner, [Shedding too much Light on a Microcontroller's Firmware Protection](https://www.usenix.org/conference/woot17/workshop-program/presentation/obermaier), USENIX WOOT 2017. STM32F0 RDP level 1.
- [Espressif security advisories](https://www.espressif.com/en/support/documents/advisories), including fault injection against secure boot and flash encryption ([CVE-2019-15894 writeup](https://www.espressif.com/en/news/Espressif_Security_Advisory_Concerning_Fault_Injection_and_Secure_Boot)).
- [CVE-2020-27211](https://nvd.nist.gov/vuln/detail/cve-2020-27211) and [Nordic security notices](https://docs.nordicsemi.com/r/bundle/struct_sa/page/struct/sa.html) (IN-133, fault against nRF52 APPROTECT). Nordic's [improved APPROTECT](https://devzone.nordicsemi.com/nordic/nordic-blog/b/blog/posts/working-with-the-nrf52-series-improved-approtect) is the hardware-plus-firmware lock on later silicon.
- Pulse Security, [Sniffing BitLocker keys from a TPM](https://pulsesecurity.co.nz/articles/TPM-sniffing). The discrete TPM bus.
- Halderman et al., [Lest We Remember: Cold Boot Attacks on Encryption Keys](https://www.usenix.org/legacy/event/sec08/tech/full_papers/halderman/halderman.pdf), USENIX Security 2008.
- Andrew Huang and Sean Cross, [The Exploration and Exploitation of an SD Memory Card](https://media.ccc.de/v/30C3_-_5294_-_en_-_saal_1_-_201312291400_-_the_exploration_and_exploitation_of_an_sd_memory_card_-_bunnie_-_xobs), 30C3, and the writeup [On Hacking MicroSD Cards](https://www.bunniestudios.com/blog/2014/on-hacking-microsd-cards/).
- Andrew Huang, [IRIS: Non-Destructive Inspection of Silicon](https://media.ccc.de/v/38c3-iris-non-destructive-inspection-of-silicon), 38C3. Looking inside a chip-scale package without decapping it.

**The startup bench**

- [NewAE ChipWhisperer](https://www.newae.com/chipwhisperer/)
- [GreatFET](https://greatscottgadgets.com/greatfet/)
- [Glasgow](https://github.com/GlasgowEmbedded/glasgow)

