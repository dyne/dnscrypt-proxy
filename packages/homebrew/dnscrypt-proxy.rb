require 'formula'

class DnscryptProxy < Formula
  url 'https://github.com/downloads/opendns/dnscrypt-proxy/dnscrypt-proxy-0.9.5.tar.gz'
  head 'https://github.com/opendns/dnscrypt-proxy.git', :branch => 'master'
  homepage 'http://www.opendns.com/technology/dnscrypt'
  md5 '725e2557408985988b21a3140dcf46aa'

  def install
    system "./configure", "--prefix=#{prefix}", "--disable-dependency-tracking"
    system "make install"
  end
end
