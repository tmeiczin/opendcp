#!/usr/bin/env python

import requests
from BeautifulSoup import BeautifulStoneSoup as Soup
import urllib
import hashlib 
import os
import re
import sys


class BinTray(object):
    def __init__(self):
        self.base_url = 'https://api.bintray.com/{section}/tmeiczin/opendcp/opendcp/{command}/'
        self.auth = None

    def version_exists(self):
        url = self.base_url.format(section='packages', command='versions')
        url += self.version
        
        r = requests.get(url, auth=self.auth)
        
        if r.ok:
            print 'version found'
            return True

    def publish(self, version):
        url = self.base_url.format(section='content', command=version)
        url += '/publish'
        
        r = requests.post(url, auth=self.auth)

        if r.ok:
            return True

        return False

    def do_upload(self, filename, version):
        url = self.base_url.format(section='content', command=version)
        url += os.path.basename(filename)
        files = {'file': open(filename, 'rb')}
        
        r = requests.put(url, files=files, auth=self.auth)

        if r.ok:
            return True

        return False

    def upload(self, files):
        for filename in files:
            basename = os.path.basename(filename)
            version = re.search('opendcp(?:-|_)(\d+.\d+.\d+)', basename).group(1)

            if self.do_upload(filename, version):
                print 'uploading %s... ok' % (filename)
                self.publish(version)
            else:
                print 'uploading %s... not ok' % (filename)


class OpenBuild(object):
    def __init__(self):
        self.base_url = 'https://api.opensuse.org/build/home:tmeiczin:opendcp'
        self.auth = None

    def _get_repositories(self):
        url = 'https://api.opensuse.org/build/home:tmeiczin:opendcp'
        
        r = requests.get(url, auth=self.auth)
        
        if not r.ok:
            return []
        
        soup = Soup(r.text)
        entries = soup.findAll('entry')
        return [x['name'] for x in entries]

    def _get_arch(self, repo):
        url = 'https://api.opensuse.org/build/home:tmeiczin:opendcp/' + repo
        
        r = requests.get(url, auth=self.auth)
        
        if not r.ok:
            return []

        soup = Soup(r.text)
        entries = soup.findAll('entry')
        
        return [x['name'] for x in entries]

    def _get_paths(self):
        data = []
        repositories = self._get_repositories()

        for repo in repositories:
            repos = {}
            repos[repo] = self._get_arch(repo)
            data.append(repos)

        return data

    def _arch(self, arch, binary):
        deb  = {'i586': 'i386', 'x86_64': 'amd64'}
        if 'deb' in binary:
            return deb[arch]
        else:
            return arch

    def _get_md5(self, link):
        md5_url = '%s.md5' % (link)

        r = requests.get(md5_url)

        if r.ok:
            md5 = r.text.split(' ')[0]
        else:
            md5 = ''

        return md5

    def links(self):
        links = []
        base = 'https://api.opensuse.org/build/home:tmeiczin:opendcp/'

        paths = self._get_paths()
        
        for path in paths:
            for repo, v in path.items():
                for arch in v:
                    url = '%s/%s/%s/OpenDCP' % (base, repo, arch)
                    r = requests.get(url, auth=self.auth)
                    soup = Soup(r.text)
                    binaries = [x['filename'] for x in soup.findAll('binary')]
                    for binary in binaries:
                        if any(ext in binary for ext in ['i386.deb', 'amd64.deb','i586.rpm', 'x86_64.rpm']):
                            link = 'http://download.opensuse.org/repositories/home:/tmeiczin:/opendcp'
                            link = '%s/%s/%s/%s' % (link, repo, self._arch(arch, binary), binary)
                            md5 = self._get_md5(link)
                            links.append({'name': repo, 'url':link, 'md5':md5})

        return links


class Publish(object):
    def __init__(self):
        self.tmp_path = '/tmp'
        self.downloaded_files = []
        
    def md5_checksum(self, filename, md5):
        with open(filename, 'rb') as fh:
            m = hashlib.md5()
            while True:
                data = fh.read(8192)
                if not data:
                    break
                m.update(data)

        if md5 == m.hexdigest():
            return True

        return False

    def get_links(self):
        print 'Getting OpenDCP URLs'
        self.ob_files = [
                {'name': 'xUbuntu_10.04',
                 'url': 'http://download.opensuse.org/repositories/home:/tmeiczin:/opendcp/xUbuntu_10.04/amd64/opendcp_0.29.0_amd64.deb',
                 'md5': '8caedb81b54b44b47484ec5978003153'},
                {'name': 'Fedora_20',
                 'url': 'http://download.opensuse.org/repositories/home:/tmeiczin:/opendcp/Fedora_20/x86_64/opendcp-0.29.0-279.3.x86_64.rpm',
                 'md5': '1c0dd3fcc51171c783f7c456334310d1'},
                ]
        self.ob_files = OpenBuild().links()

    def download(self):
        self.downloaded_files = []
        self.get_links()

        for l in self.ob_files:
            basename = os.path.basename(l['url'])
            m = re.search(r'opendcp(?:-|_)(\d+.\d+.\d+)', basename)
            if not m:
                print basename + ' bad version parse'
                continue

            version = m.group(1)
            search = '%s' % (version)
            if 'deb' in basename:
                replacement = '%s_%s' % (version, l['name'])
            else:
                replacement = '%s-%s' % (version, l['name'])
            filename = '%s/%s' % (self.tmp_path, re.sub(search, replacement, basename).lower())

            urllib.urlretrieve (l['url'], filename)

            if self.md5_checksum(filename, l['md5']):
                print 'downloading %s... ok' % (filename)
                self.downloaded_files.append(filename)
            else:
                print 'downloading %s... not ok' % (filename)

    def upload(self):
        bintray = BinTray()
        bintray.upload(self.downloaded_files)

package = None
if len(sys.argv) > 1:
    package = sys.argv[1]

print '---------------'
print 'OpenDCP Publish'
print '---------------'
p = Publish()

if not package:
    print 'Downloading OpenDCP files from build.opensuse.org'
    p.download()
else:
    p.downloaded_files = [package]

print '\nUploading OpenDCP files to bintray.com'
p.upload()
