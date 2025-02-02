#########################################################################
#    growhouse-server_1.0.0.bb: Recipe for growhose-server and service  #
#    Copyright(C) 2018                                                  #
#########################################################################

SUMMARY = "Application which talks to awsapp, zigbee-coordinator and ble server"
LICENSE = "CLOSED"

SRC_URI = "file://growhouse-server/ file://growhouse-server.service"

inherit systemd

S = "${WORKDIR}"

DEPENDS += "mosquitto cjson"

FILES_${PN} = "/usr/sbin/"

INSANE_SKIP_${PN} = "ldflags"

EXTRA_OEMAKE = "'CFLAGS=${CFLAGS}'"

do_compile() {
	cd growhouse-server
	oe_runmake LINKER_FLAGS=${LDFLAGS}
}

do_install() {
	install -d ${D}/usr/sbin/
	install -m 0755 ${S}/growhouse-server/growhouse-server ${D}/usr/sbin/

	install -d ${D}${sysconfdir}/systemd/system
	install -m 0644 ${S}/growhouse-server.service ${D}${sysconfdir}/systemd/system/
}

#INSANE_SKIP_${PN} += "file-rdeps"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "growhouse-server.service"
SYSTEMD_AUTO_ENABLE = "enable"
