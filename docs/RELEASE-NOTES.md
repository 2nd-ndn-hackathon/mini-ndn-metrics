Release Notes
=============

Mini-NDN version 0.1.0 (initial release)
----------------------------------------

Release date: July 15, 2015

Mini-NDN is a lightweight networking emulation tool that enables testing, experimentation, and
research on the NDN platform. Mini-NDN uses the
[Named Data Networking Forwarding Daemon (NFD)](https://github.com/named-data/NFD)
and the [Named Data Link State Routing Protocol (NLSR)](https://github.com/named-data/NLSR) to
emulate an NDN network on a single system.

**Included features**:

- Run a complete NDN network on a single system

- Automatic configuration of NLSR to provide a routable NDN network

- Supports user created NDN applications

- Create a topology using the included Mini-NDN Edit GUI application

- Allows individual configuration of NFD and NLSR parameters for each node

- Provides an experiment management framework for easy creation of custom networking experiments

- Uses a simple topology file format to define hosts, links, and configuration values

- Configure network link parameters including bandwidth, delay, and loss rate

- Includes a pre-configured topology file to replicate the NDN testbed
