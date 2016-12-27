#! /usr/bin/env python

import re
import urllib2


def parse_blacklist(content, trusted=False):
    rx_comment = re.compile(r'^(#|$)')
    rx_u = re.compile(r'^@*\|\|([a-z0-9.-]+[.][a-z]{2,})\^?(\$(popup|third-party))?$')
    rx_l = re.compile(r'^([a-z0-9.-]+[.][a-z]{2,})$')
    rx_h = re.compile(r'^[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}\s+([a-z0-9.-]+[.][a-z]{2,})$')
    rx_mdl = re.compile(r'^"[^"]+","([a-z0-9.-]+[.][a-z]{2,})",')
    rx_b = re.compile(r'^([a-z0-9.-]+[.][a-z]{2,}),.+,[0-9: /-]+,')
    rx_trusted = re.compile(r'^([*a-z0-9.-]+)$')

    names = set()
    rx_set = [rx_u, rx_l, rx_h, rx_mdl, rx_b]
    if trusted:
        rx_set = [rx_trusted]
    for line in content.splitlines():
        line = str.lower(str.strip(line))
        if rx_comment.match(line):
            continue
        for rx in rx_set:
            matches = rx.match(line)
            if not matches:
                continue
            name = matches.group(1)
            names.add(name)
    return names


def blacklist_from_url(url):
    req = urllib2.Request(url)
    trusted = False
    if req.get_type() == "file":
        trusted = True
    response = urllib2.urlopen(req, timeout=10)
    if trusted is False and response.getcode() != 200:
        print("# HTTP return code: {}".format(response.getcode()))
        return set()
    content = response.read()
    return parse_blacklist(content, trusted)


def name_cmp(name):
    parts = name.split('.')
    parts.reverse()
    return str.join('.', parts)


def blacklists_from_config_file(file):
    global_names = set()
    with open(file) as fd:
        for line in fd:
            line = str.strip(line)
            if str.startswith(line, "#") or line == "":
                continue
            url = line
            names = blacklist_from_url(url)
            print("\n\n########## Blacklist from {} ##########\n".format(url))
            redundant = names & global_names
            if redundant:
                print("# Ignored entries already present in previous lists: {}\n".format(len(redundant)))
            names = names - global_names
            global_names = global_names | names
            names = list(names)
            names.sort(key=name_cmp)
            for name in names:
                print(name)


blacklists_from_config_file("domains-blacklist.conf")
