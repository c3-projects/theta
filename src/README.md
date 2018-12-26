# theta
_a fast, secure and spanning communiction system_

## Concept
Theta provides secure and fast communication between peers as long as a route can be traced between
compliant nodes. This provides a greater degree of communication than the internet, as IP requires
that each node has a registered address, which is registered with the same registry as the receiver.
Theta, like many other systems, circumvents this problem by using hashed public keys as the address.

## Model
Theta defines an abstraction called a medium, which has the following properties:
1. It can send _and_ receive binary data, with a >50% data rate
2. It is not controlled by time-travelling or mind-reading gods with supercomputers
or a positive solution to P = NP

The crypto community as a whole is pretty comfortable with the latter for all mediums.
If this turns out to be a false assumption, then we will all be too busy dying as civilisation crumbles
to care about the security of our messages.

Medium that fulfil the former include:
* The internet
* Human speech
* Carrier pigeons
* Trains
* People carrying usb sticks
* Smoke signals

However, a lot of things can be done to make theta's life easier and faster, such as:
* Integrity checks
* Half-duplex collision checking or duplex channels
* Reliabilty
* A general resistance to people screwing with it

## Higher level media

A pseudochannel is a medium that can determine where a message begins.

A channel is a pseudochannel that can determine length of a message.

A half-link is a channel that guarantees integrity (the message is correct)

A link is a half-link that guarantees reliability (the message will get to the destination)

A conversation is a link with the following guarenteees:
* Privacy (the message cannot be read by anyone outside the conversation)
* Security (the message cannot be changed by anyone outside the conversation)
* Authenticity (the message cannot be created by anyone outside the conversation)
* Deniability (the message cannot be verified by anyone outside the conversation)

Deniability is created by the fact that the sender and receiver use the same key

If the message fails any of the guarentees, an exception will be raised on the sender
(and optionally the receiver)

Please note that it is not theta's job to negotiate the channel structure.

## Developing
The library internals should be relatively easy to read,
but the external API _must_ be easy to use.

AviD's Rule of Usability:
> Security at the expense of usability comes at the expense of security.

There is a good chance that if your code doesn't look nice, you have overcomplicated things.

The code must also be optionally fast. Note the ordering in that sentence.
It is not optional for the code to be fast; rather it is the library user's decision
whether or not to make the code fast. Where possible, give the option for static data structures.

Macros get a deservedly large hate train. However, when used correctly,
they can massively improve the readabilty, reliability and maintainablity of the code.

For instance, if you need to make a large number of almost-identical classes (e.g. the hash functions),
then don't write the same code over and over again, as no-one will be able to fix what you've messed up.

Make sure you undefine macros after they're needed. If they are likely to be reused,
provide `<macro set>` and `clean_<macro set>` headers.

## TODO
* Port libc and libc++ to a distributed platform
