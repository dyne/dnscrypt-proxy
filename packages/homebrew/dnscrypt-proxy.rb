require 'formula'

class DnscryptProxy < Formula
  url 'https://github.com/downloads/opendns/dnscrypt-proxy/dnscrypt-proxy-0.10.tar.gz'
  head 'https://github.com/opendns/dnscrypt-proxy.git', :branch => 'master'
  homepage 'http://www.opendns.com/technology/dnscrypt'
  md5 '975c71d82b9b3f16a2ab69d2c3b4686c'

  def install
    system "./configure", "--prefix=#{prefix}", "--disable-dependency-tracking"
    system "make install"
  end
end
