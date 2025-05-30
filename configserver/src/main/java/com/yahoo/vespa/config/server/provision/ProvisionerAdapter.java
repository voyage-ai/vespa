// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.provision;

import com.yahoo.config.model.api.HostProvisioner;
import com.yahoo.config.model.api.Provisioned;
import com.yahoo.config.provision.ApplicationId;
import com.yahoo.config.provision.Capacity;
import com.yahoo.config.provision.ClusterSpec;
import com.yahoo.config.provision.HostSpec;
import com.yahoo.config.provision.ProvisionLogger;
import com.yahoo.config.provision.Provisioner;

import java.util.List;


/**
 * A wrapper for {@link Provisioner} to avoid having to expose multitenant
 * behavior to the config model. Adapts interface from a {@link HostProvisioner} to a
 * {@link Provisioner}.
 *
 * @author Ulf Lilleengen
 */
public class ProvisionerAdapter implements HostProvisioner {

    private final Provisioner provisioner;
    private final ApplicationId applicationId;
    private final Provisioned provisioned;

    public ProvisionerAdapter(Provisioner provisioner, ApplicationId applicationId, Provisioned provisioned) {
        this.provisioner = provisioner;
        this.applicationId = applicationId;
        this.provisioned = provisioned;
    }

    @Override
    public HostSpec allocateHost(String alias) {
        // TODO: Remove this method since hosted/non-hosted needs different interfaces. See also ModelContextImpl.getHostProvisioner
        // Illegal argument because we end here by following a path not suitable for the environment steered
        // by the content of the nodes tag in services
        throw new IllegalArgumentException("Clusters in hosted environments must have a <nodes count='N'> tag " +
                                                "matching all zones, and having no <node> subtags, " +
                                                "see https://docs.vespa.ai/en/reference/services.html");
    }

    @Override
    public List<HostSpec> prepare(ClusterSpec cluster, Capacity capacity, ProvisionLogger logger) {
        provisioned.add(cluster, capacity);
        return provisioner.prepare(applicationId, cluster, capacity, logger);
    }

}
