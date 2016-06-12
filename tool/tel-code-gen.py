#!/usr/bin/env python3

# 2016, Georg Sauthoff <mail@georg.so>
# GPLv3+

import os
import sys
import argparse
import logging

import pycountry as pyc
import phonenumbers as p
import phonenumbers.geocoder as g
import phonenumbers.carrier as c
import wikitextparser as wtp
import re
import collections
import requests

try:
    import colorlog
    have_colorlog = True
except ImportError:
    have_colorlog = False

def mk_arg_parser():
    p = argparse.ArgumentParser(description='Generate CC/MCC/MNC/... tables')
    p.add_argument('--all', action='store_true', default = False,
        help = 'generate all tables')
    p.add_argument('--cc', action='store_true', default = False,
        help = 'generate country calling code table')
    p.add_argument('--area', action='store_true', default = False,
        help = 'generate area code table')
    p.add_argument('--carrier', action='store_true', default = False,
        help = 'generate carrier code table')
    p.add_argument('--mcc', action='store_true', default = False,
        help = 'generate mobile country code table')
    p.add_argument('--mnc', action='store_true', default = False,
        help = 'generate mobile network code table')
    p.add_argument('--cur', action='store_true', default = False,
        help = 'generate iso currency table')
    p.add_argument('--alpha3', action='store_true', default = False,
        help = 'generate iso currency table')
    p.add_argument('-o', '--output', metavar = 'FILE', required = True,
        help = 'output directory')
    p.add_argument('--dir', metavar = 'DIR', default = '.',
        help = 'download or input directory')
    p.add_argument('--nodl', action = 'store_true', default = False,
        help = 'disable downloading')
    p.add_argument('--intkey', action = 'store_true', default = False,
        help = 'use integer keys and accessor methods where it makes sense')
    return p

arg_parser = mk_arg_parser()

def mk_logger():
    log = logging.getLogger(__name__)
    #log = colorlog.getLogger(__name__)
    log.setLevel(logging.DEBUG)

    #'%(asctime)s - %(name)s - %(levelname)-8s - %(message)s'
    #'%(asctime)s - %(levelname)-8s - %(message)s'
    format = '%(asctime)s - %(levelname)-8s - %(message)s'
    cformat = '%(log_color)s' + format
    date_format = '%Y-%m-%d %H:%M:%S'
    f = logging.Formatter(format, date_format)
    if have_colorlog == True:
        cf = colorlog.ColoredFormatter(cformat, date_format,
            log_colors = { 'DEBUG': 'reset', 'INFO': 'reset',
                'WARNING': 'bold_yellow' , 'ERROR': 'bold_red',
                'CRITICAL': 'bold_red'})

    else:
        cf = f

    ch = logging.StreamHandler()
    #ch = colorlog.StreamHandler()
    ch.setLevel(logging.DEBUG)
    if os.isatty(2):
        ch.setFormatter(cf)
    else:
        ch.setFormatter(f)
    log.addHandler(ch)

    #fh = logging.FileHandler('foo.log')
    #fh.setLevel(logging.DEBUG)
    #fh.setFormatter(f)
    #log.addHandler(fh)

    return log

log = mk_logger()


def extract_tables_wt(wt):
  r = []
  for s in wt.sections:
    if 'International' in s.title:
        ts = [ s.title ]
        ccs = []
    else:
        ts = s.title.split(' - ')
        if ts.__len__() < 2:
          continue
        cc = ts[1].strip()
        if cc == '':
          continue
        ccs = [cc]
        if '/' in cc:
            ccs = cc.split('/')
        elif '-' in cc:
            ccs = cc.split('-')
    if s.tables.__len__() == 0:
        continue
    for row in s.tables[0].getdata():
        if row[0].strip() == '':
            continue
        r.append(row + [ccs, ts[0]])
  return r

def wiki_row_to_list(l):
    t = l[1:]
    t = t.strip()
    x = t.split('||')
    if x.__len__() == 0 or x[0].strip() == '':
        return []
    return [ a.strip() for a in x ]

def extract_tables(ls):
    r = []
    cc = None
    country = None
    in_table = False
    for l in ls:
        if l.find('==') == 0:
            t = l.strip("= ")
            xs = t.split(' - ')
            if xs.__len__() > 1:
                country = xs[0]
                cc = re.split('[/-]', xs[1])
                #print(country + ' ' + cc)
            else:
                cc = None
                country = None
        elif l.find('{|') == 0:
            in_table = True
        elif l.find('|}') == 0:
            in_table = False
        elif in_table and l.find('|-') != 0 and l.__len__() > 0 and l[0] == '|':
            t = wiki_row_to_list(l)
            if t.__len__() == 0:
                continue
            r.append(t + [cc, country])
    return r

def aggregate_mcc(ls):
    h = collections.OrderedDict()
    for l in ls:
        #print(l)
        if l[0] in h:
            seen = False
            for i in h[l[0]]:
                if i[-2] == l[-2]:
                    seen = True
            if seen == True:
                continue
            h[l[0]].append(l[-2:])
        else:
            h[l[0]] = [l[-2:]]
    return h

inofficial_iso_cc = {
        'XK' : 'Kosovo',
        'AB' : 'Abkhazia'
        }

def translate_cc(cc):
    try:
        return pyc.countries.get(alpha2=cc).name
    except:
        if cc in inofficial_iso_cc:
            return inofficial_iso_cc[cc]
        else:
            return 'other'

def translate_cc_list(ccs):
    if ccs:
        return '/'.join([translate_cc(cc) for cc in ccs])
    else:
        return 'other'

def mk_mcc_to_country_map(mcc_map):
    h = {}
    for (mcc, rhs) in mcc_map.items():
        ccs = []
        for i in rhs:
            if not i[-2]:
                continue
            ccs = ccs + i[-2]
        h[mcc] =  translate_cc_list(ccs)
    h['001'] = 'TEST'
    h['901'] = 'International'
    r = sorted(h.items(), key=lambda t : t[0])
    return r

# The idea with the intkey option:
# exploit that Lua hash tables include an optimization for integer keys,
# i.e. they are stored separately without the index
# however, test show that the intkey code doesn't pay it is even slower
# than all string hashing, even with all keys as ints (sparse array)
# are len < 5
#
# Profiling shows that Lua hash table implementation is quite efficient
# and that the cost of a function call in Lua is realtively high, in general
def pp_lua_map(comments, name, h, o, args = None):
    if args and args.intkey:
        h = sorted(h, key=lambda t : (t[0].__len__(), t[0]))
    print('', file=o)
    cs = comments.splitlines()
    for c in cs:
        print('-- ' + c, file=o)
    print(name + ' = {', file=o)
    for (mcc, country) in h:
      if args and args.intkey and mcc.__len__() < 4:
        print('''  [{}] = '{}','''.format(mcc, country.strip("'").replace("'", "\\'")), file=o)
      else:
        print('''  ['{}'] = '{}','''.format(mcc, country.strip("'").replace("'", "\\'")), file=o)
    print('}', file=o)
    if args and args.intkey:
        print('''function get_cc_map(s)
  if string.len(s) < 4 then
    return cc_map[tonumber(s)]
  else
    return cc_map[s]
  end
end''', file=o)

def pp_lua_mcc_map(h, o):
    log.info('Generating mobile country code lua table')
    comments = '''MCC - Mobile Country Code
extracted from: https://en.wikipedia.org/wiki/Mobile_country_code'''
    pp_lua_map(comments, 'mcc_map', h, o)


def format_operator(s):
    r = ''
    p = wtp.parse(s)
    if p.wikilinks.__len__() >= 1:
        t = p.wikilinks[0].text
        if t:
            r = t
        else:
            r = p.wikilinks[0].target
    elif p.external_links.__len__() >= 1:
        r = p.external_links[0].text
    else:
        r = s
    r = re.sub('<[^>]*>', '', r)
    r = r.replace('&amp;', '&').strip()
    return r

def aggregate_mcc_mnc(rows):
    h = []
    for row in rows:
        s = row[2]
        if 'Unassigned' in row[3]:
            continue
        if s == '':
            s = row[3]
        if s == '':
            continue
        if row[1] == '' or row[1] == '?' or '-' in row[1]:
            continue
        h.append( (row[0]+row[1], format_operator(s)) )
    h = sorted(h, key=lambda t : t[0])
    return h

def pp_lua_mcc_mnc_map(h, o):
    log.info('Generating mobile network code lua table')
    comments = '''MNC - Mobile Network Code
extracted from: https://en.wikipedia.org/wiki/Mobile_country_code'''
    pp_lua_map(comments, 'mcc_mnc_map', h, o)

def aggregate_iso_alpha3():
    h = []
    for c in pyc.countries:
        h.append( (c.alpha3, c.name) )
    h = sorted(h, key=lambda t : t[0])
    return h

def pp_lua_iso_alpha3_map(h, o):
    log.info('Generating iso alpha3 country code lua table')
    comments = '''generated with pycountry
cf. https://bitbucket.org/flyingcircus/pycountry'''
    pp_lua_map(comments, 'iso_alpha3_map', h, o)

def aggregate_iso_cur():
    h = []
    for c in pyc.currencies:
        h.append( (c.letter, c.name) )
    h.append(('SDR', pyc.currencies.get(letter='XDR').name))
    h = sorted(h, key=lambda t : t[0])
    return h

def pp_lua_iso_cur_map(h, o):
    log.info('Generating iso currencies lua table')
    comments = '''Generated with pycountry
cf. https://bitbucket.org/flyingcircus/pycountry

In addition, it includes SDR as popular XDR alias. '''
    pp_lua_map(comments, 'cur_map', h, o)


def translate_region(x):
    if x == 'AC':
        return 'Ascension Island'
    elif x == 'TA':
        return 'Tristan'
    else:
        return pyc.countries.get(alpha2=x).name

def mk_cc_map():
    h = []
    for (code, xs) in p.COUNTRY_CODE_TO_REGION_CODE.items():
        if code in p.COUNTRY_CODES_FOR_NON_GEO_REGIONS:
            continue
        h.append((code, '/'.join([translate_region(x) for x in xs])))
    m = [(800, 'International Freephone (UIFN)'), (808, 'Shared Cost Services (reserved)'), (870, 'Inmarsat'), (878, 'Universal Personal Telecommunications'), (881, 'Global Mobile Satellite System'), (882, 'International Networks'), (883, 'International Networks'), (888, 'Telecommunications for Disaster Relief by OCHA'), (979, 'International Premium Rate Service')]
    h = sorted(h+m, key=lambda t : t[0])
    return h

def pp_lua_cc_map(h, o, args):
    log.info('Generating calling code lua table')
    comments = '''Country Calling Codes a.k.a. Country Codes (CC)'
extracted from https://en.wikipedia.org/wiki/List_of_country_calling_codes#Alphabetical_listing_by_country_or_region
The map also contain de-facto country codes, i.e. prefixes resulting
from the ITU CC and a numbering plan area code that addresses
countries, territories or other international entities.
Google's libphonenumber was not used because it just contains ITU CCs'''
    pp_lua_map(comments, 'cc_map', h, o, args)

def mk_libphone_map(l):
    h = []
    for (i, j) in l:
        h.append((i, j.get('en', list(j.values())[0])))
    h = sorted(h, key=lambda t : t[0])
    return h

def mk_area_code_map():
    return mk_libphone_map(g.GEOCODE_DATA.items())

def mk_carrier_code_map():
    return mk_libphone_map(c.CARRIER_DATA.items())

def pp_lua_area_code_map(h, o):
    log.info('Generating area calling code lua table')
    comments = '''Generated from Google's libphonenumber via python-phonenumbers
cf. https://github.com/daviddrysdale/python-phonenumbers/
    https://github.com/googlei18n/libphonenumber'''
    pp_lua_map(comments, 'area_code_map', h, o)

def pp_lua_carrier_code_map(h, o):
    log.info('Generating carrier code lua table')
    comments = '''Generated from Google's libphonenumber via python-phonenumbers
cf. https://github.com/daviddrysdale/python-phonenumbers/
    https://github.com/googlei18n/libphonenumber

Note that due to number portability effective in different areas
those (original) carrier allocations are not necessarily stable.'''
    pp_lua_map(comments, 'carrier_code_map', h, o)

def extract_cc(wt):
    def get_ccs(link):
        r = []
        for i in [ link.target, link.text ]:
            if i and i.strip().startswith('+'):
                r.append([ x.strip()[1:].replace(' ', '') for x in i.split(',') ])
        r = sorted(r, key=lambda t : (t.__len__(), t[0].__len__()))
        if r:
            return r[-1]
        else:
            return []

    h = {}
    section = None
    for x in wt.sections:
        if 'Alphabetical listing' in x.title:
            section = x
    rows = section.tables[0].getdata()[1:]
    for row in rows:
        for link in wtp.parse(row[1]).wikilinks:
            ccs = get_ccs(link)
            for cc in ccs:
                if cc:
                    if cc in h:
                        if row[0] in h[cc]:
                            continue
                        h[cc] = h[cc] + '/' + row[0]
                    else:
                      h[cc] = row[0]
    h = sorted(h.items(), key=lambda t : t[0])
    return h

wp_url = 'http://en.wikipedia.org/w/index.php?title={}&action=raw'

def fetch(fname, url, args):
    a = '{}/{}'.format(args.dir, fname)
    if args.nodl:
        log.info('Opening {}'.format(a))
        f = open(a)
        return f.read()
    else:
        log.info('Fetching {}'.format(url))
        r = requests.get(url)
        log.debug('HTTP status: {}'.format(r.status_code))
        with open(a, 'w') as f:
            f.write(r.text)
        return r.text

def run(args):
    with open(args.output, 'w') as output:
        print('''-- Lookup tables for various mobile telecommunications related codes
--
-- Autogenerated with tel-code-gen.py
''', file = output)
        if args.all:
            args.cc      = True
            args.cur     = True
            args.alpha3  = True
            args.carrier = True
            args.area    = True
            args.mcc     = True
            args.mnc     = True
        if args.cur:
            iso_cur_map = aggregate_iso_cur()
            pp_lua_iso_cur_map(iso_cur_map, output)
        if args.alpha3:
            iso_alpha3_map = aggregate_iso_alpha3()
            pp_lua_iso_alpha3_map(iso_alpha3_map, output)
        if args.carrier:
            carrier_code_map = mk_carrier_code_map()
            pp_lua_carrier_code_map(carrier_code_map, output)
        if args.area:
            area_code_map = mk_area_code_map()
            pp_lua_area_code_map(area_code_map, output)
        if args.cc:
            t = fetch('cc.wtext', wp_url.format('List_of_country_calling_codes'),
                    args)
            wu = wtp.parse(t)
            cc_map = extract_cc(wu)
            pp_lua_cc_map(cc_map, output, args)
        if args.mcc or args.mnc:
            s = fetch('mcc.wtext', wp_url.format('Mobile_country_code'), args)
            wt = wtp.parse(s)
            rows = extract_tables_wt(wt)
            if args.mcc:
                mcc_map = aggregate_mcc(rows)
                mcc_to_country_map = mk_mcc_to_country_map(mcc_map)
                pp_lua_mcc_map(mcc_to_country_map, output)
            if args.mnc:
                mcc_mnc_map = aggregate_mcc_mnc(rows)
                pp_lua_mcc_mnc_map(mcc_mnc_map, output)


def main():
    args = arg_parser.parse_args()
    run(args)

def imain(argv):
    args = arg_parser.parse_args(argv)
    run(args)

if __name__ == '__main__':
    sys.exit(main())





#cc_map = mk_cc_map()
#pp_lua_cc_map(cc_map)





