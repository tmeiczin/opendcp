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

        with open(filename, 'rb') as f:
            r = requests.put(url, data=f, auth=self.auth)

        if r.ok:
            return True

        return False

    def upload(self, files):
        for filename in files:
            basename = os.path.basename(filename)
            version = re.search(
                'opendcp(?:-|_)(\d+.\d+.\d+)',
                basename).group(1)

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
        deb = {'i586': 'i386', 'x86_64': 'amd64'}
        rpm = {'i586': 'i686', 'x86_64': 'x86_64'}
        if 'deb' in binary:
            return deb[arch]
        else:
            return rpm[arch]

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
                        if any(
                            ext in binary for ext in [
                                'i386.deb',
                                'amd64.deb',
                                'i586.rpm',
                                'i686.rpm',
                                'x86_64.rpm']):
                            link = 'http://download.opensuse.org/repositories/home:/tmeiczin:/opendcp'
                            link = '%s/%s/%s/%s' % (link,
                                                    repo,
                                                    self._arch(
                                                        arch,
                                                        binary),
                                                    binary)
                            md5 = self._get_md5(link)
                            links.append(
                                {'name': repo, 'url': link, 'md5': md5})

        return links


class Publish(object):

    def __init__(self):
        self.tmp_path = '/tmp'
        self.downloaded_files = []

    def md5_checksum(self, filename, md5):
        if not md5:
            print 'No MD5 found, skipping'
            return True

        with open(filename, 'rb') as f:
            m = hashlib.md5()
            while True:
                data = f.read(8192)
                if not data:
                    break
                m.update(data)
        f.close()

        if md5 != m.hexdigest():
            print 'MD5 mismatch %s (%s -> %s)' % (filename, md5, m.hexdigest())
            return False

        return True

    def replace_os(self, filename):
        filename = filename.replace('centos_centos-6', 'centos_6')
        filename = filename.replace('redhat_rhel-6', 'rhel_6')
        if 'opensuse' in filename:
            filename = re.sub(
                r'(opendcp-\d+.\d+.\d+-\w+_\d+.\d+)(-\d+.\d+)',
                r'\1',
                filename)
        else:
            filename = re.sub(
                r'(opendcp-\d+.\d+.\d+-\w+_\d+)(-\d+.\d+)',
                r'\1',
                filename)

        return filename

    def get_links(self):
        print 'Getting OpenDCP URLs'
        self.ob_files = OpenBuild().links()

    def download_file(self, url, filename):
        with open(filename, 'wb') as f:
            r = requests.get(url, stream=True)

            if not r.ok:
                print 'bad http request %s %s (%s)' % (url, filename, r.status_code)
                return False

            for block in r.iter_content(1024):
                if not block:
                    break
                f.write(block)
        f.close()

        return True

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
            filename = '%s/%s' % (self.tmp_path,
                                  re.sub(
                                      search,
                                      replacement,
                                      basename).lower())
            filename = self.replace_os(filename)

            if not self.download_file(l['url'], filename):
                continue

            if self.md5_checksum(filename, l['md5']):
                print 'downloading %s... ok' % (filename)
                self.downloaded_files.append(filename)
            else:
                print 'downloading %s... not ok' % (filename)
                self.downloaded_files.append(filename)

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
