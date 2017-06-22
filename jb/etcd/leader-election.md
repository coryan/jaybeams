# Leader Election Based on etcd

JayBeams needed a leader election protocol implementation.  These are
my notes about how the protocol is implemented in etcd clients.
The motivation for leader election protocols is beyond the scope of
this document, but briefly:
leader election (sometimes "Master Election") is a distributed systems
protocol used to construct fault-tolerant, highly-available,
distributed systems.
The problem to solve is how to select one replica between a group to
perform some actions, and to automatically select an alternative when
the selected replica fails or terminates.
This problem often arises when one needs to handle a workload that
cannot be trivially sharded or load balanced across replicas.
For example, because the workload has strict ordering constraints.

In this case, the leader election protocol can be used to
automatically select one of the replicas as the leader.
The remaining replicas wait until the leader fails or terminates, when
they run the protocol again to select a new leader.
While waiting the replicas might chose to witness the activity of the
leader for faster recovery, but this is not necessary and not part of
the protocol itself.

Using a leader election protocol obviates the need to configure which
one of the replicas will be the leader, and minimizes the need for
human intervention on a failure.

This implementation of leader election is based on the Go client
libraries for etcd found in
[github.com/coreos/etcd](https://github.com/coreos/etcd/).

## Underlying primitives

### Leases

leases create session abstractions, they need to be refreshed or they
expire, need async server for them ...

### Key-Values

etcd stores key values. the keys and values are byte arrays, though
typically UTF-8 strings.
They key-values have a number of revision counters associated with
them.
The "version" counter is the number of times the key has been modified.
The "create revision" is etcd revision when the key was created.
The "mod revision" is the etcd revision when the key was last modified.

The "etcd revision" is incremented each time *anything* changes in
etcd.  So effectively it serves as a total order for changes.

### Keys Associated with a Lease

etcd stores key-value pairs.  The keys are just a byte array.  The
key-values can be associated with a lease.  When the lease expires
the key-values are automatically deleted.

### Test-and-Set

etcd supports test-and-set (and compare-and-swap).  The user can
describe what action to take if a key-value pair has a specific value,
and what action to take if it does not.
The action to take can be to *get* the current value.

### Querying

The user can query the value of a key or range of keys.
The query can filter by "creation revision".
The query can sort by key, or "creation revision".
The query can limit the number of results.

So one can express the query "find the 1 key-value pair with the
highest revision count that is lower than X" (where X can be "my
revision count").

### Watching

The user can "watch" for events (delete, modifications, creations) in
a range of keys or a single key.

### The Basic Algorithm

The election needs a name, each participant will key a node prefixed
with the name of the election.

Each participant in leader election requests a lease, that will serve
as the session, when the participant terminates (or asks for the lease
to be revoked) the protocol can assume the participant is really dead.

The key for each participant is constructed by joining the election
name and the session id.
The key-value pair is associated with the lease, so it is
automatically deleted by etcd when the lease expires.

The participant whose key-value pair has the *lowest* creation
revision is the leader.

The participants wait until all keys with a lower creation revision
are deleted.  This wait is trivial for the leader, there is nothing to
wait for.

### Avoiding Thundering Herds

To avoid large queries and large watching scenarios the participants
find the key with the highest creation revision that is still lower
than their own.
They only wait until that one key is deleted.

No keys can be inserted between that key and their own because all new
keys have a higher creation number then the one they have.

### Query Leader

To find out the current leader a client simply queries for all the
keys with the given election name as prefix.
Then sort by creation key ascending.
Then limit to 1 response.
That yields the key and value for the current leader.

### The Node Values

Each participant stores a value in their node.
The value is not relevant for the leader election protocol.
Clients may want to know the value 