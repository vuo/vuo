<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:import href="/usr/local/Cellar/docbook-xsl/1.78.1/docbook-xsl/xhtml5/chunk.xsl"/>
	<xsl:template name="user.head.content">
		<xsl:variable name="codefile" select="document('../VuoManualHeader.xhtml',/)"/>
		<xsl:copy-of select="$codefile/head/node()"/>
	</xsl:template>
	<xsl:template name="user.header.navigation">
		<xsl:variable name="codefile" select="document('../VuoManualNavigation.xhtml',/)"/>
		<xsl:copy-of select="$codefile/body/node()"/>
	</xsl:template>
</xsl:stylesheet>
