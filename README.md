JayBeams Documentation                {#mainpage}
======================

Another project to have fun coding.

The JayBeams library performs relative time delay estimation of market
feeds.  That is, given two feeds for the same inside quote data it
estimates, in near real-time which one is faster, and by how much.

- Licensing details are found in the LICENSE file.
- The installation instructions are in the INSTALL file.

### Motivation

The US equity markets have a lot of redundant feeds with basically the
same information, or where one feed is a super set of a second set.
When one has two feeds an interesting question is to know how much
faster is one feed vs. the other, or rather, how much faster it is
right now, because the latency changes over time.  There are (usually)
no message IDs or any other simple way to match events in one feed
vs. events in the second feed.  So the problem of "time-delay
estimation" requires some heavier computation, and doing this in
real-time is extra challenging.  The current implementation is based
on cross-correlation of the two signals, using FFTs on GPUs to
efficiently implement the cross-correlations.

#### Really, that is the movation?

I confess, I wanted to learn how to program GPUs, and given my
background this appeared as an interesting application.

#### So where is the code?

I am pushing the code slowly.  I want to make sure it compiles and
passes its tests on at least a couple of Linux variants.  This can be
a challenge given the dependency on OpenCL libraries.

### What is with the name?

Jay Beams is named after a [WWII electronic warfare](https://en.wikipedia.org/wiki/List_of_World_War_II_electronic_warfare_equipment) system.
The name was selected more or less at random from the Wikipedia list
of such systems, and does not mean anything in particular.
