require 'formula'

class DnscryptProxy < Formula
  url 'https://github.com/downloads/opendns/dnscrypt-proxy/dnscrypt-proxy-0.8.tar.gz'
  head 'https://github.com/opendns/dnscrypt-proxy.git', :branch => 'master'
  homepage 'http://www.opendns.com/technology/dnscrypt'
  md5 'cfde647dbcaa6b11f7b3c645bb866434'

  def install
    system "./configure", "--prefix=#{prefix}"
    system "make install"
  end
end
