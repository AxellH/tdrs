# Weltraum TDRS
Weltraum TDRS is a lightweight ZeroMQ-based event hub.

## A what?

TDRS allows distribution/publishing of events (basically messages) to a large amount of subscribers.

![TDRS overview](README/tdrs01@2x.png)

## Okay, how does it work?

Every component (`C`) is connected to the `TDRS` service using a [ZeroMQ](http://zeromq.org) [`ZMQ_SUB` socket](http://api.zeromq.org/4-0:zmq-socket#toc10). Additionally, each component eventually connects to the `TDRS` service using a dedicated [`ZMQ_REQ` socket](http://api.zeromq.org/4-0:zmq-socket#toc4). This connection allows each component to submit an event that gets broadcasted to all subscribed components, **including itself**. By its very nature, the publisher/subscriber-pattern does not allow for the subscribers to directly return a result to the component triggering the event. Therefor, this architecture expects components to react to each other using only events (instead of direct communication), pushing the whole infrastructure-design into a more micro-service-oriented direction.

## Can I haz a GIF?

Sure.

![TDRS](README/tdrs01.gif)

## How can I build it?

```bash
$ autoreconf -i
```

```bash
$ ./configure
```

```bash
$ make
```

## How can I run it?

```bash
$ ./tdrs --receiver-listen "tcp://*:19890" --publisher-listen "tcp://*:19891"
```

## How can I scale it?

```bash
$ ./tdrs --receiver-listen "tcp://*:19790" --publisher-listen "tcp://*:19791" --chain-link "tcp://127.0.0.1:19891" --chain-link "tcp://127.0.0.1:19991"
```

```bash
$ ./tdrs --receiver-listen "tcp://*:19890" --publisher-listen "tcp://*:19891" --chain-link "tcp://127.0.0.1:19791" --chain-link "tcp://127.0.0.1:19991"
```

```bash
./tdrs --receiver-listen "tcp://*:19990" --publisher-listen "tcp://*:19991" --chain-link "tcp://127.0.0.1:19891" --chain-link "tcp://127.0.0.1:19791"
```

## What does `TDRS` stand for?

It stands for ["Tracking and data relay satellite"](https://en.wikipedia.org/wiki/Tracking_and_data_relay_satellite). We at [Weltraum](https://weltraum.co) like to give our components names that sort of fit their function inside our infrastructure, from an astronautics point of view.
