require 'formula'

class DnscryptProxy < Formula
  url 'https://github.com/downloads/opendns/dnscrypt-proxy/dnscrypt-proxy-0.10.tar.gz'
  head 'https://github.com/opendns/dnscrypt-proxy.git', :branch => 'master'
  homepage 'http://www.opendns.com/technology/dnscrypt'
  md5 '41f1f3d9d876bf82e2eeccaded585cc0'

  def install
    system "./configure", "--prefix=#{prefix}", "--disable-dependency-tracking"
    system "make install"
  end
end
