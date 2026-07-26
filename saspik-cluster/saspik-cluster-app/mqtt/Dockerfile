FROM eclipse-mosquitto:2.0

COPY mosquitto.conf /mosquitto/config/mosquitto.conf
COPY mosquitto.acl /mosquitto/config/mosquitto.acl
COPY passwordfile /mosquitto/config/passwordfile

# Set proper permissions for config files (mosquitto user uid 1883)
RUN chmod 0600 /mosquitto/config/mosquitto.acl && \
    chmod 0600 /mosquitto/config/passwordfile && \
    chown mosquitto:mosquitto /mosquitto/config/mosquitto.acl && \
    chown mosquitto:mosquitto /mosquitto/config/passwordfile

EXPOSE 1883 9001

CMD ["/usr/sbin/mosquitto", "-c", "/mosquitto/config/mosquitto.conf"]