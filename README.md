JayBeams Documentation                {#mainpage}
======================

Another project to have fun coding.

The JayBeams library performs time delay analysis of market quotes.
The US equity markets have a lot of redundant feeds with basically the
same information, or where one feed is a super set of a second set.
When one has two feeds an interesting question is to know how much
faster is one feed vs. the other, or rather, how much faster it is
right now, because the latency changes over time.  There are (usually)
no message IDs or any other simple way to match events in one feed
vs. events in the second feed.  So the problem of "time-delay
analysis" requires some heavier computation, and doing this in
real-time is extra challenging.  The current implementation is based
on cross-correlation of the two signals, using FFTs on GPUs to
efficiently implement the cross-correlations.

- Licensing details are found in the LICENSE file.
- The installation instructions are in the INSTALL file.

### What is with the name?

Jay Beams is named after a [WWII electronic warfare](https://en.wikipedia.org/wiki/List_of_World_War_II_electronic_warfare_equipment) system.
The name was selected more or less at random from the Wikipedia list
of such systems, and does not mean anything in particular.

