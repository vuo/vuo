class CheckAllTheThings < Formula
  url 'https://github.com/smokris/check-all-the-things.git',
      :revision => '0006fc0618e2d42f69a009770040645b903ccb83'
  version '0'

  def install
    prefix.install Dir['*']
  end

  test do
    system "#{prefix}/check-all-the-things", '--help'
  end
end
