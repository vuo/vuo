class Pandoc < Formula
  url "https://github.com/jgm/pandoc/releases/download/1.12.0.2/pandoc-1.12.0.2.dmg"
  sha256 "97d31c0944f32c5ff97613f1d770a7fc2df5b94047689a8b5974fba46157fe8b"

  def install
    system "hdiutil mount pandoc-1.12.0.2.dmg"
    system "gzcat < '/Volumes/pandoc\ 1.12.0.2/pandoc-1.12.0.2.pkg/Contents/Archive.pax.gz' | pax -r -s '/.\\/usr\\/local\\///'"
    system "hdiutil unmount '/Volumes/pandoc\ 1.12.0.2'"
    bin.install "bin/pandoc"
  end

  test do
    system "#{bin}/pandoc", "--version"
  end
end
