# nagios2graphite

`nagios2graphite` is a nagios event broker module that provides nagios information on an UDP graphite server

** Still in Alpha version - Hack away **

currently supports service and host checks

## Requirements
Configure graphite with UDP enabled. In your `carbon.cfg`:

    ENABLE_UDP_LISTENER = True
    UDP_RECEIVER_INTERFACE = 127.0.0.1
    UDP_RECEIVER_PORT = 2003

Why UDP? We don't want our monitoring system to become unstable if our graphite goes down do we?

## Compilation
Requires basic compile tools to be installed.

    make
    make install

## Configuration
Configure broker events and add the broker module to your `nagios.cfg`.

    event_broker_options=-1 # Yes that's -1
    broker_module=/var/lib/nagios3/nagios2graphite.o

This will send to localhost on port 2003. To change this supply:

    broker_module=/var/lib/nagios3/nagios2graphite.o host="somehost.org" port=2003

This will send to `somehost.org` on port 2003. To activate debug:

    broker_module=/var/lib/nagios3/nagios2graphite.o host="somehost.org" port=2003

Note: If you already have a broker_module line, you need to ADD another line. Nagios will define multiple brokers active.

## Loading
You need to restart your nagios config if you changed this lines.

Check in `/var/log/nagios3/nagios.log` to see if the module is loaded

  [1348838041] graphite: nagios2graphite by Patrick Debois
  [1348838041] graphite: successfully finished initialization
  [1348838041] Event broker module '/var/lib/nagios3/nagios2graphite.o' initialized successfully.

## Sample debug output

    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.current_attempt 3 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.max_attempts 3 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.state_type 1 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.state 2 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.execution_time 1.002262 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.early_timeout 0 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.latency 0.243000 1348836352
    [1348836352] graphite: nagios.vagrant-client.servicecheck.free_memory.return_code 2 1348836352

## Todo

- more robust error handling (hey my C is rusty :)
- expose more internal information: f.i. NOTIFICATION-DATA, DOWNTIME-DATA, FLAPPING-DATA, EXTERNAL-COMMAND-DATA, CONTACT-NOTIFICATION-DATA - See <http://nagios.sourceforge.net/download/contrib/documentation/misc/NEB%202x%20Module%20API.pdf>
- Correctly handle metrics that change their metric size (us,ms,s) or (mb,gb,tb) : See <http://nagiosplug.sourceforge.net/developer-guidelines.html#AEN201>

## Alternatives

- Graphios: python based - <https://github.com/shawn-sterling/graphios> to graphite
- Gearphite: python based - <https://github.com/ahhdem/gearphite> to opentsdb
- Icinga-to-graphite: <https://github.com/roidelapluie/icinga-to-graphite>

