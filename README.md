## JayBeams Documentation

[![Join the chat at https://gitter.im/jaybeams/Lobby](https://badges.gitter.im/jaybeams/Lobby.svg)](https://gitter.im/jaybeams/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.org/coryan/jaybeams.svg?branch=master)](https://travis-ci.org/coryan/jaybeams)
[![coveralls](https://coveralls.io/repos/coryan/jaybeams/badge.svg?branch=master&service=github)](https://coveralls.io/github/coryan/jaybeams?branch=master)
[![codecov](https://codecov.io/gh/coryan/jaybeams/branch/master/graph/badge.svg)](https://codecov.io/gh/coryan/jaybeams)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/79c4108849884f4cb71e70597089f9cf)](https://www.codacy.com/app/coryan/jaybeams?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=coryan/jaybeams&amp;utm_campaign=Badge_Grade)
[![Coverity Scan](https://img.shields.io/coverity/scan/12813.svg)](https://scan.coverity.com/projects/jaybeams2)
[![Documentation](https://img.shields.io/badge/documentation-master-brightgreen.svg)](http://coryan.github.io/jaybeams/)

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

You can find more details about the motivation, and the performance
requirements on [my posts](htts://coryan.github.io/)

#### Really, that is the movation?

I confess, I wanted to learn how to program GPUs, and given my
background this appeared as an interesting application.

#### So where is the code?

I am pushing the code slowly.  I want to make sure it compiles and
passes its tests on at least a couple of Linux variants.  This can be
a challenge given the dependency on OpenCL libraries.

### What is with the name?

JayBeams is named after a [WWII electronic warfare](https://en.wikipedia.org/wiki/List_of_World_War_II_electronic_warfare_equipment) system.
The name was selected more or less at random from the Wikipedia list
of such systems, and is not meant to represent anything in
particular.  It sounds cool, in a Flash Gordon kind of way.

## Licensing and Copyright Notice

Copyright 2015-2017 Carlos O'Ryan

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
